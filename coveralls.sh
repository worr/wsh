#!/bin/sh

if [ "$CFLAGS" = "" ] && [ "$CC" = "gcc" ]; then
   pip install --user cpp-coveralls
   coveralls --exclude server/test --exclude library/test --exclude util --exclude client/test --exclude build --gcov-options '\-lp'
fi
