#!/bin/sh

if [ "$CFLAGS" = "" ] && [ "$CC" = "gcc" ]; then
   pip install --user cpp-coveralls
   coveralls -y .coveralls.yml -r .. --exclude server/test --exclude library/test --exclude util --exclude client/test --exclude build/CMakeFiles --exclude build/gen --gcov-options '\-lp'
fi
