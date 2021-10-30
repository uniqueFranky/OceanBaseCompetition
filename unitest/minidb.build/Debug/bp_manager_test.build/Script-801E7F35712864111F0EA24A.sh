#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/franky/documents/github/wyy/unitest
  /Applications/CMake.app/Contents/bin/cmake -D TEST_TARGET=bp_manager_test -D TEST_EXECUTABLE=/Users/franky/documents/github/wyy/bin/Debug/bp_manager_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/Users/franky/documents/github/wyy/unitest -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=bp_manager_test_TESTS -D CTEST_FILE=/Users/franky/documents/github/wyy/unitest/bp_manager_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /Applications/CMake.app/Contents/share/cmake-3.22/Modules/GoogleTestAddTests.cmake
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/franky/documents/github/wyy/unitest
  /Applications/CMake.app/Contents/bin/cmake -D TEST_TARGET=bp_manager_test -D TEST_EXECUTABLE=/Users/franky/documents/github/wyy/bin/Release/bp_manager_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/Users/franky/documents/github/wyy/unitest -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=bp_manager_test_TESTS -D CTEST_FILE=/Users/franky/documents/github/wyy/unitest/bp_manager_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /Applications/CMake.app/Contents/share/cmake-3.22/Modules/GoogleTestAddTests.cmake
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/franky/documents/github/wyy/unitest
  /Applications/CMake.app/Contents/bin/cmake -D TEST_TARGET=bp_manager_test -D TEST_EXECUTABLE=/Users/franky/documents/github/wyy/bin/MinSizeRel/bp_manager_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/Users/franky/documents/github/wyy/unitest -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=bp_manager_test_TESTS -D CTEST_FILE=/Users/franky/documents/github/wyy/unitest/bp_manager_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /Applications/CMake.app/Contents/share/cmake-3.22/Modules/GoogleTestAddTests.cmake
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/franky/documents/github/wyy/unitest
  /Applications/CMake.app/Contents/bin/cmake -D TEST_TARGET=bp_manager_test -D TEST_EXECUTABLE=/Users/franky/documents/github/wyy/bin/RelWithDebInfo/bp_manager_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/Users/franky/documents/github/wyy/unitest -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=bp_manager_test_TESTS -D CTEST_FILE=/Users/franky/documents/github/wyy/unitest/bp_manager_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /Applications/CMake.app/Contents/share/cmake-3.22/Modules/GoogleTestAddTests.cmake
fi

