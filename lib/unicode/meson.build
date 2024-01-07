codegen_byte_size = executable('codegen_byte_size', 'codegen/byte_size.c')
codegen_char_width = executable('codegen_char_width', 'codegen/char_width.c')

# curl -o codegen/EastAsianWidth.txt https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
double_width_table = files('codegen/EastAsianWidth.txt')
# curl -o codegen/UnicodeData.txt https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
zero_width_table = files('codegen/UnicodeData.txt')

utf8_table_c = custom_target(
    'utf8_table',
    output: 'utf8_table.c',
    command: [codegen_byte_size, 'utf8'],
    capture: true,
)

utf16_table_c = custom_target(
    'utf16_table',
    output: 'utf16_table.c',
    command: [codegen_byte_size, 'utf16'],
    capture: true,
)

double_width_table_c = custom_target(
    'double_width_table',
    output: 'double_width_table.c',
    command: [
        codegen_char_width,
        double_width_table,
        'cx__double_width',
        '1',
        '8',
    ],
    capture: true,
)

zero_width_table_c = custom_target(
    'zero_width_table',
    output: 'zero_width_table.c',
    command: [codegen_char_width, zero_width_table, 'cx__zero_width', '2', '9'],
    capture: true,
)

unicode_src = [
    files(
        'unicode-width.c',
        'utf16.c',
        'utf8.c',
    ),
    utf8_table_c,
    utf16_table_c,
    double_width_table_c,
    zero_width_table_c,
]
