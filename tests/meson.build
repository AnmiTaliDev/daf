test_names = ['buffer', 'search', 'encoding', 'filetype', 'editor']

foreach name : test_names
  test_exe = executable(
    'test_' + name,
    'test_' + name + '.c',
    include_directories: daf_inc,
    link_with: daf_lib,
  )
  test(name, test_exe)
endforeach
