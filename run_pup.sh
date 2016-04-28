#!/bin/bash

# Run pup, and install pup if it is not present.

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

PUP_TMP="$DIR/scripts/pup.whl.tmp"
PUP="$DIR/scripts/pup.whl"

set +e

# Update pup if needed.
curl -o ".pup-version-new" https://pup.pedigree-project.org/pup-version
if cmp --silent ".pup-version-new" ".pup-version"; then
    rm -rf ".pup-version-new"
else
    mv ".pup-version-new" ".pup-version"
    curl -o "$PUP_TMP" https://pup.pedigree-project.org/pup.whl && \
        mv "$PUP_TMP" "$PUP"
fi

set -e

python "$PUP/pedigree_updater" --config="$DIR/scripts/pup/pup.conf" $*

