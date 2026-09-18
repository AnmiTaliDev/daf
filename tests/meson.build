test_names = [
  'buffer', 'search', 'encoding', 'filetype', 'editor', 'undo', 'clipboard',
  'replace', 'autoindent', 'theme', 'config', 'syntax',
]

foreach name : test_names
  test_exe = executable(
    'test_' + name,
    'test_' + name + '.c',
    include_directories: daf_inc,
    link_with: daf_lib,
    dependencies: [tree_sitter_dep, dl_dep],
  )
  test(name, test_exe)
endforeach
