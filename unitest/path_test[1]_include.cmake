if(EXISTS "/Users/franky/documents/github/wyy/unitest/path_test[1]_tests.cmake")
  include("/Users/franky/documents/github/wyy/unitest/path_test[1]_tests.cmake")
else()
  add_test(path_test_NOT_BUILT path_test_NOT_BUILT)
endif()