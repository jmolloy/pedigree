#!/bin/bash

GIT_ROOT=$(git rev-parse --show-toplevel)

# Run the Python-based testsuites.
python2 -m unittest discover -s $GIT_ROOT/tests -p "*_tests.py"
