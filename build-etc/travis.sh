#!/bin/bash

# Handle running on Travis-CI.
TRAVIS_OPTIONS=
if [ ! -z "$TRAVIS" ]; then
    TRAVIS_OPTIONS="travis=$TRAVIS"

    # The toolchain .deb file installs all files owned as root, which is not
    # useful when we want to symlink our crt* and POSIX headers. So, fix that.
    ME=`whoami`
    sudo chown -R "$ME" $script_dir/pedigree-compiler
fi
