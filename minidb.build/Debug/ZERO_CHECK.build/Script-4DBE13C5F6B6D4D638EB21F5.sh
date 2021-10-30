#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/franky/documents/github/wyy
  make -f /Users/franky/documents/github/wyy/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/franky/documents/github/wyy
  make -f /Users/franky/documents/github/wyy/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/franky/documents/github/wyy
  make -f /Users/franky/documents/github/wyy/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/franky/documents/github/wyy
  make -f /Users/franky/documents/github/wyy/CMakeScripts/ReRunCMake.make
fi

