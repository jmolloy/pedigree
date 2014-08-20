#!/bin/bash

ROOT=$(git rev-parse --show-toplevel)

# Prepare the build.
"$ROOT/build-etc/prepare-deb.sh" "$ROOT"

# Kick off the build.
pushd "$ROOT/build-etc/travis-compilers"
debuild -S -sa
#dpkg-buildpackage
popd
