project('libcextras', 'c', version: '0.1', default_options: ['warning_level=3'])

threads_dep = dependency('threads', required: get_option('threads'))

subdir('lib')
subdir('include')


libcextras_dep = declare_dependency(
    include_directories: include_directories('include'),
    link_with: libcextras,
    dependencies: threads_dep,
)

subdir('testlib')
subdir('tests')
