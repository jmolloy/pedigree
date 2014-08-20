#!/bin/bash

# Prepares for Debian package builds by 

if [ -z "$1" ]; then
    echo "Please specify the path to the Pedigree repository."
    exit 1
fi

PEDIGREE=`realpath "$1"`

TARGET_DIR="$PEDIGREE/build-etc/travis-compilers"
TARGET_TAR="$PEDIGREE/build-etc/travis-compilers_1.0.7.orig.tar.bz2"

[ -e "$TARGET_TAR" ] && echo "Source tarball already exists, not creating." && exit 0

rm -rf "$TARGET_DIR/compilers" "$TARGET_DIR/scripts"
mkdir -p "$TARGET_DIR/scripts"

# The actual toolchain build is fairly small. No need to pull the entire repo.
cp -R "$PEDIGREE/compilers" "$TARGET_DIR/"
cp "$PEDIGREE/scripts/checkBuildSystemNoInteractive.pl" "$TARGET_DIR/scripts/"

# Clean up the target directory after recursive copies.
rm "$TARGET_DIR/compilers/dir"

# Clean up any past builds.
rm -rf "$TARGET_DIR/pedigree-compiler"

# But now drop in our download cache - can't download these on the PPA build
# systems, which is a nuisance.
mkdir -p "$TARGET_DIR/pedigree-compiler/dl_cache"
cp "$PEDIGREE"/pedigree-compiler/dl_cache/*.bz2 "$TARGET_DIR/pedigree-compiler/dl_cache/"

# Create a Makefile to make building easier.
cp "$PEDIGREE/compilers/Makefile.deb" "$TARGET_DIR/Makefile"

# Create an .orig.bz2 for this now so the build kicks off properly.
pushd "$TARGET_DIR"
rm -f "$TARGET_TAR"
tar -cjf "$TARGET_TAR" compilers scripts pedigree-compiler Makefile
rm -rf pedigree-compiler
popd
