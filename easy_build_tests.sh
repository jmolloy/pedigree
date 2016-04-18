#!/bin/bash

# Script that can be run to set up a Pedigree repository for building with minimal
# effort.

set -e

echo "Pedigree Easy Build"
echo "NOTE: This Easy Build script only builds tools that run on your build " \
    "system; this does not build Pedigree itself. Unless you are specifically" \
    " working on tests or benchmarks, you probably want one of the other Easy" \
    " Build scripts."

scons build_tests_only=1
