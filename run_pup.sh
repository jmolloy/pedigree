#!/bin/bash

# Run pup, and install pup if it is not present.

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

PUP_TMP="$DIR/scripts/pup.whl.tmp"
PUP="$DIR/scripts/pup.whl"

set +e

try_update_pup()
{
    curl -o ".pup-version-new" https://pup.pedigree-project.org/pup-version
    if cmp --silent ".pup-version-new" ".pup-version"; then
        rm -f ".pup-version-new"
    else
        curl -o "$PUP_TMP" https://pup.pedigree-project.org/pup.whl && \
            mv "$PUP_TMP" "$PUP" && mv ".pup-version-new" ".pup-version"
    fi
}

# Download pup for the first time if we don't know what version we have.
if [ ! -e .pup-version ]; then
    try_update_pup
fi

# Update pup if needed.
if test $(find .pup-version -maxdepth 1 -mmin +120); then
    try_update_pup
fi

set -e

python "$PUP/pedigree_updater" --config="$DIR/scripts/pup/pup.conf" $*

