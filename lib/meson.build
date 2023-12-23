subdir('collection')

subdir('concurrency')

subdir('memory')

subdir('unicode')

src = [concurrency_src, collection_src, memory_src, unicode_src]

cextras = static_library(
    'cextras',
    src,
    build_by_default: false,
    c_args: ['-fvisibility=hidden'],
)
