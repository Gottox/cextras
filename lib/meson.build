subdir('collection')

subdir('concurrency')

src = [concurrency_src, collection_src]

libcextras = static_library('cextras', src, c_args: ['-fvisibility=hidden'])
