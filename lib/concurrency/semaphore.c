#include "../../include/cextras/concurrency.h"

int
cx_semaphore_init(struct CxSemaphore *semaphore, size_t count) {
	int rv = 0;

	rv = pthread_mutex_init(&semaphore->mutex, NULL);
	if (rv != 0) {
		goto out;
	}

	rv = pthread_cond_init(&semaphore->cond, NULL);
	if (rv != 0) {
		pthread_mutex_destroy(&semaphore->mutex);
		goto out;
	}

	semaphore->count = count;

out:
	return rv;
}

int
cx_semaphore_wait(struct CxSemaphore *semaphore) {
	int rv = 0;

	rv = pthread_mutex_lock(&semaphore->mutex);
	if (rv != 0) {
		goto out;
	}

	while (semaphore->count == 0) {
		rv = pthread_cond_wait(&semaphore->cond, &semaphore->mutex);
		if (rv != 0) {
			pthread_mutex_unlock(&semaphore->mutex);
			goto out;
		}
	}
	semaphore->count--;

	rv = pthread_mutex_unlock(&semaphore->mutex);
	if (rv != 0) {
		goto out;
	}

out:
	return rv;
}

int
cx_semaphore_post(struct CxSemaphore *semaphore) {
	int rv = 0;

	rv = pthread_mutex_lock(&semaphore->mutex);
	if (rv != 0) {
		goto out;
	}

	semaphore->count++;
	rv = pthread_cond_signal(&semaphore->cond);
	if (rv != 0) {
		pthread_mutex_unlock(&semaphore->mutex);
		goto out;
	}

	rv = pthread_mutex_unlock(&semaphore->mutex);
	if (rv != 0) {
		goto out;
	}

out:
	return rv;
}

int
cx_semaphore_destroy(struct CxSemaphore *semaphore) {
	int rv = 0;

	rv = pthread_mutex_destroy(&semaphore->mutex);
	if (rv != 0) {
		goto out;
	}

	rv = pthread_cond_destroy(&semaphore->cond);

out:
	return rv;
}
