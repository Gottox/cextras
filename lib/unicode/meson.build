table_codegen = executable('table_codegen', 'table_codegen.c')

utf8_table_h = custom_target(
    'utf8_table',
    output: 'utf8_table.h',
    command: [table_codegen, 'utf8'],
    capture: true,
)

utf16_table_h = custom_target(
    'utf16_table',
    output: 'utf16_table.h',
    command: [table_codegen, 'utf16'],
    capture: true,
)

unicode_src = [
    files(
        'utf16.c',
        'utf8.c',
    ),
    utf8_table_h,
    utf16_table_h,

]
