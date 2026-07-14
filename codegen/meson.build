radixgen = executable('radixgen', 'radixgen.c', native: true, install: false)

# usage: embed_gen.process('input.txt', extra_args: 'var_name')
embed_exec = executable('embed', 'embed.c', native: true)
embed_gen = generator(
    embed_exec,
    arguments: ['@INPUT@', '@OUTPUT0@', '@OUTPUT1@', '@EXTRA_ARGS@'],
    output: ['@BASENAME@.c', '@BASENAME@.h'],
)
