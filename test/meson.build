add_languages('cpp', native: false)

tests = [
    'collection/buffer_test.c',
    'collection/collector.c',
    'collection/hash_map_test.c',
    'collection/lru_test.c',
    'collection/pin_vec_test.c',
    'collection/radix_tree.c',
    'collection/rc_hash_map_test.c',
    'collection/rc_map_test.c',
    'collection/rc_radix_tree_test.c',
    'collection/vec_test.c',
    'concurrency/future_test.c',
    'concurrency/threadpool_test.c',
    'endian.c',
    'macro.c',
    'memory/prealloc_pool.c',
    'memory/rc.c',
    'memory/utils.c',
    'testlib.c',
    'testlib.cpp',
    'unicode.c',
]

foreach test : tests
    test_exe = executable(
        test.underscorify(),
        test,
        install: false,
        dependencies: [
            testlib_dep,
            cextras_dep,
        ],
    )
    test(
        test,
        test_exe,
        args: ['-t'],
        protocol: 'tap',
        timeout: 5,
    )
endforeach
