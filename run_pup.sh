#!/bin/bash

# Run pup, and install pup if it is not present.

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

PUP_TMP="$DIR/scripts/pup.whl.tmp"
PUP="$DIR/scripts/pup.whl"

set +e

# Update pup if needed.
curl -o "$PUP_TMP" https://pup.pedigree-project.org/pup.whl
if cmp --silent "$PUP_TMP" "$PUP"; then
    rm -f "$PUP_TMP"
else
    mv "$PUP_TMP" "$PUP"
fi

set -e

python "$PUP/pedigree_updater" --config="$DIR/scripts/pup/pup.conf" $*

