#!/bin/bash

# Run pup, and install pup if it is not present.

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

if [ ! -e "$DIR/scripts/pup.whl" ]; then
  # TODO(miselin): figure out just how to update this later.
  curl -o "$DIR/scripts/pup.whl" https://pup.pedigree-project.org/pup.whl
fi

python "$DIR/scripts/pup.whl/pedigree_updater" --config="$DIR/scripts/pup/pup.conf" $*

