#!/bin/bash

# Pre-commit script. Will check license on committed files and fix if this is
# deemed necessary. Should be able to do so without user intervention.

ROOT=$(git rev-parse --show-toplevel)

git diff --cached --name-only | xargs python $ROOT/fixlicense.py -r 'git add -u' -f

