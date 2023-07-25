tests = [
    'concurrency/threadpool_test.c',
    'collection/buffer_test.c',
    'collection/lru_test.c',
    'collection/rc_hash_map_test.c',
    'collection/rc_map_test.c',
]

foreach test : tests
    test_exe = executable(
        test.underscorify(),
        test,
        install: false,
        dependencies: [
            testlib_dep,
            libcextras_dep,
        ],
    )
    test(test, test_exe)
endforeach
