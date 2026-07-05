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

subdir('src')
subdir('tests')
