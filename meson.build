project(
  'daf',
  'c',
  version: '0.2.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'buildtype=debugoptimized',
  ],
)

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')
add_project_arguments(
  '-DDAF_DEFAULT_THEME="' + get_option('default_theme') + '"',
  '-DDAF_CONFIG_SUBPATH="' + get_option('config_subpath') + '"',
  '-DDAF_THEME_DIR_SUBPATH="' + get_option('theme_dir_subpath') + '"',
  language: 'c',
)

tree_sitter_dep = dependency('tree-sitter')
cc = meson.get_compiler('c')
dl_dep = cc.find_library('dl', required: false)

subdir('src')
subdir('tests')
