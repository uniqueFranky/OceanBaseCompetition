#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/franky/documents/github/wyy/deps/common
  /Applications/CMake.app/Contents/bin/cmake -E cmake_symlink_library /Users/franky/documents/github/wyy/lib/Debug/libcommon.1.0.0.dylib /Users/franky/documents/github/wyy/lib/Debug/libcommon.1.dylib /Users/franky/documents/github/wyy/lib/Debug/libcommon.dylib
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/franky/documents/github/wyy/deps/common
  /Applications/CMake.app/Contents/bin/cmake -E cmake_symlink_library /Users/franky/documents/github/wyy/lib/Release/libcommon.1.0.0.dylib /Users/franky/documents/github/wyy/lib/Release/libcommon.1.dylib /Users/franky/documents/github/wyy/lib/Release/libcommon.dylib
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/franky/documents/github/wyy/deps/common
  /Applications/CMake.app/Contents/bin/cmake -E cmake_symlink_library /Users/franky/documents/github/wyy/lib/MinSizeRel/libcommon.1.0.0.dylib /Users/franky/documents/github/wyy/lib/MinSizeRel/libcommon.1.dylib /Users/franky/documents/github/wyy/lib/MinSizeRel/libcommon.dylib
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/franky/documents/github/wyy/deps/common
  /Applications/CMake.app/Contents/bin/cmake -E cmake_symlink_library /Users/franky/documents/github/wyy/lib/RelWithDebInfo/libcommon.1.0.0.dylib /Users/franky/documents/github/wyy/lib/RelWithDebInfo/libcommon.1.dylib /Users/franky/documents/github/wyy/lib/RelWithDebInfo/libcommon.dylib
fi

