#!/bin/bash

# Run clang-tidy over the source tree.
#
# Requires you to have run bear with scons first:
# $ bear scons

echo >tidy.log
find src -regextype egrep -regex ".*\.(cc|c|cpp)$" -exec clang-tidy {} \; >>tidy.log
