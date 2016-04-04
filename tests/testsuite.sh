#!/bin/bash

set -e

GIT_ROOT=$(git rev-parse --show-toplevel)

# Run the Python-based testsuites.
/usr/bin/env python -m unittest discover -s $GIT_ROOT/tests -p "*_tests.py"

if [ ! -x "$GIT_ROOT/build/host/testsuite" ]; then
    echo "C/C++ testsuite not present, not attempting to run it."
    exit 0
fi

# Remove old coverage data.
find "$GIT_ROOT" -type f -name '*.gcda' -delete

# Run under valgrind if we can.
WRAPPER=
if type valgrind >/dev/null 2>&1; then
    WRAPPER="valgrind --tool=memcheck --leak-check=full --error-exitcode=1"
fi

# Run the C++ testsuites.
$WRAPPER "$GIT_ROOT/build/host/testsuite"

if [ "x$TRAVIS" != "x" ]; then
    echo "On Travis, not running coverage."
    exit 0
fi

mkdir -p "$GIT_ROOT/coverage"

# Coverage failing is not a problem; but make sure to filter only to src/ so
# we don't pull in e.g. all of the gtest code.
gcovr -d --object-directory="$GIT_ROOT/build" -r "$GIT_ROOT" \
    --gcov-filter="src/*" --html --html-details --exclude=".*test\-.*" \
    --exclude=".*buildutil/.*" -o "$GIT_ROOT/coverage/index.html" || exit 0
