daf_lib_sources = files(
  'common.c',
  'terminal.c',
  'encoding.c',
  'filetype.c',
  'theme.c',
  'config.c',
  'buffer.c',
  'search.c',
  'render.c',
  'input.c',
  'editor.c',
  'syntax.c',
)

daf_inc = include_directories('.')

daf_lib = static_library(
  'daf_lib',
  daf_lib_sources,
  include_directories: daf_inc,
  dependencies: [tree_sitter_dep, dl_dep],
)

executable(
  'daf',
  'main.c',
  include_directories: daf_inc,
  link_with: daf_lib,
  dependencies: [tree_sitter_dep, dl_dep],
  install: true,
)
