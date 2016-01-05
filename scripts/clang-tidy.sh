#!/bin/bash

# Run clang-tidy over the source tree.
#
# Requires you to have run bear with scons first:
# $ bear scons

echo >tidy.log
if [ "x$SRCPATH" = "x" ]; then
    SRCPATH=src
fi
find $SRCPATH -regextype egrep -regex ".*\.(cc|c|cpp)$" -print0 | xargs -0 clang-tidy '-header-filter=.*' >>tidy.log
