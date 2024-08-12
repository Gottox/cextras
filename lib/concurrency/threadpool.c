/******************************************************************************
 *                                                                            *
 * Copyright (c) 2023, Enno Boland <g@s01.de>                                 *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions are     *
 * met:                                                                       *
 *                                                                            *
 * * Redistributions of source code must retain the above copyright notice,   *
 *   this list of conditions and the following disclaimer.                    *
 * * Redistributions in binary form must reproduce the above copyright        *
 *   notice, this list of conditions and the following disclaimer in the      *
 *   documentation and/or other materials provided with the distribution.     *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS    *
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  *
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR     *
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,      *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,        *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING       *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                            *
 ******************************************************************************/

#include "../../include/cextras/concurrency.h"
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <unistd.h>

static int
cpu_count(void) {
	long numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
	return (int)numCPUs;
}

static struct CxTask *
task_new(
		struct CxThreadpool *threadpool, cx_threadpool_task_t function,
		void *arg) {
	pthread_mutex_lock(&threadpool->task_pool_mutex);
	struct CxTask *task = cx_prealloc_pool_get(&threadpool->task_pool);
	pthread_mutex_unlock(&threadpool->task_pool_mutex);

	if (task == NULL) {
		return NULL;
	}

	task->function = function;
	task->arg = arg;
	task->next = NULL;

	return task;
}

static void
task_free(struct CxThreadpool *threadpool, struct CxTask *task) {
	pthread_mutex_lock(&threadpool->task_pool_mutex);
	cx_prealloc_pool_recycle(&threadpool->task_pool, task);
	pthread_mutex_unlock(&threadpool->task_pool_mutex);
}

static struct CxTask *
worker_get_task(struct CxWorker *worker) {
	struct CxTask *task = worker->head;
	if (task != NULL) {
		worker->head = task->next;
		if (worker->head == NULL) {
			worker->tail = NULL;
		}
		atomic_fetch_sub(&worker->queue_length, 1);
	}
	return task;
}

static struct CxTask *
worker_find_task(struct CxWorker *worker) {
	struct CxThreadpool *threadpool = worker->pool;
	struct CxTask *task = NULL;

	size_t worker_count = threadpool->worker_count;
	size_t worker_index = worker - threadpool->workers;
	for (size_t i = 0; i < worker_count && task == NULL; i++) {
		struct CxWorker *candidate =
				&worker->pool->workers[(i + worker_index) % worker_count];
		pthread_mutex_lock(&candidate->queue_mutex);
		task = worker_get_task(candidate);
		pthread_mutex_unlock(&candidate->queue_mutex);
	}
	return task;
}

static struct CxTask *
worker_wait_for_task(struct CxWorker *worker) {
	struct CxThreadpool *threadpool = worker->pool;
	struct CxTask *task = NULL;

	pthread_mutex_lock(&worker->queue_mutex);

	task = worker_get_task(worker);
	if (task == NULL) {
		atomic_fetch_sub(&threadpool->active_workers, 1);
		pthread_cond_signal(&worker->pool->wait_cond);

		while (task == NULL && atomic_load(&worker->pool->running)) {
			pthread_cond_wait(&worker->queue_cond, &worker->queue_mutex);
			task = worker_get_task(worker);
		}

		atomic_fetch_add(&threadpool->active_workers, 1);
	}
	pthread_mutex_unlock(&worker->queue_mutex);

	return task;
}

static void *
worker_run(void *data) {
	struct CxWorker *worker = data;
	struct CxThreadpool *threadpool = worker->pool;
	struct CxTask *task = NULL;

	while (atomic_load(&threadpool->running)) {
		task = worker_find_task(worker);
		if (task == NULL) {
			task = worker_wait_for_task(worker);
		}
		if (task != NULL) {
			task->function(task->arg);
			task_free(threadpool, task);
		}
	}
	return 0;
}

static int
worker_cleanup(struct CxWorker *worker) {
	pthread_cond_signal(&worker->queue_cond);
	pthread_join(worker->thread, NULL);
	pthread_mutex_destroy(&worker->queue_mutex);
	pthread_cond_destroy(&worker->queue_cond);
	return 0;
}

static int
worker_init(struct CxWorker *worker, struct CxThreadpool *threadpool) {
	int rv = 0;
	worker->pool = threadpool;
	worker->head = NULL;
	worker->tail = NULL;
	atomic_init(&worker->queue_length, 0);

	rv = pthread_mutex_init(&worker->queue_mutex, NULL);
	if (rv != 0) {
		rv = -1;
		goto out;
	}
	rv = pthread_cond_init(&worker->queue_cond, NULL);
	if (rv != 0) {
		rv = -1;
		goto out;
	}

	rv = pthread_create(&worker->thread, NULL, worker_run, worker);

out:
	if (rv < 0) {
		worker_cleanup(worker);
	}
	return 0;
}

int
cx_threadpool_init(struct CxThreadpool *threadpool, size_t worker_count) {
	int rv = 0;
	if (worker_count == 0) {
		worker_count = cpu_count();
	}

	cx_prealloc_pool_init(&threadpool->task_pool, sizeof(struct CxTask));

	threadpool->worker_count = worker_count;
	atomic_init(&threadpool->active_workers, worker_count);
	atomic_init(&threadpool->running, true);

	threadpool->workers = calloc(worker_count, sizeof(struct CxWorker));
	if (threadpool->workers == NULL) {
		rv = -1;
		goto out;
	}

	for (size_t i = 0; i < worker_count; i++) {
		struct CxWorker *worker = &threadpool->workers[i];
		rv = worker_init(worker, threadpool);
		if (rv < 0) {
			goto out;
		}
	}

out:
	if (rv < 0) {
		cx_threadpool_cleanup(threadpool);
	}
	return 0;
}

int
cx_threadpool_schedule(
		struct CxThreadpool *threadpool, cx_threadpool_task_t function,
		void *arg) {
	int rv = 0;
	size_t min_queue_length = SIZE_MAX;
	struct CxWorker *worker = NULL;
	struct CxTask *new_task = task_new(threadpool, function, arg);
	if (new_task == NULL) {
		goto out;
	}

	for (size_t i = 0; i < threadpool->worker_count; ++i) {
		struct CxWorker *candidate = &threadpool->workers[i];
		size_t queue_length = atomic_load(&candidate->queue_length);
		if (queue_length < min_queue_length) {
			worker = candidate;
			min_queue_length = queue_length;
		}
	}

	rv = pthread_mutex_lock(&worker->queue_mutex);

	if (worker->tail != NULL) {
		worker->tail->next = new_task;
	} else {
		worker->head = new_task;
	}
	worker->tail = new_task;

	atomic_fetch_add(&worker->queue_length, 1);
	pthread_cond_signal(&worker->queue_cond);

out:
	pthread_mutex_unlock(&worker->queue_mutex);
	if (rv < 0) {
		task_free(threadpool, new_task);
	}
	return 0;
}

int
cx_threadpool_wait(struct CxThreadpool *threadpool) {
	pthread_mutex_lock(&threadpool->wait_mutex);
	while (atomic_load(&threadpool->active_workers) > 0) {
		pthread_cond_wait(&threadpool->wait_cond, &threadpool->wait_mutex);
	}
	pthread_mutex_unlock(&threadpool->wait_mutex);
	return 0;
}

int
cx_threadpool_cleanup(struct CxThreadpool *threadpool) {
	atomic_store(&threadpool->running, false);
	for (size_t i = 0; i < threadpool->worker_count; i++) {
		struct CxWorker *worker = &threadpool->workers[i];
		worker_cleanup(worker);
	}
	free(threadpool->workers);
	cx_prealloc_pool_cleanup(&threadpool->task_pool);

	return 0;
}
