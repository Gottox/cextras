src = []

if get_option('collection')
    subdir('collection')
    src += collection_src
endif

if get_option('concurrency')
    subdir('concurrency')
    src += concurrency_src
endif

if get_option('memory')
    subdir('memory')
    src += memory_src
endif

if get_option('unicode')
    subdir('unicode')
    src += unicode_src
endif

cextras = static_library(
    'cextras',
    src,
    build_by_default: false,
    c_args: ['-fvisibility=hidden'],
)
