#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: release.sh build_path [output_dir] [name]" >&2
    exit 1
fi

RELEASE=1
if [ -z "$KEYCDN_KEYPATH" ]; then
    echo "KEYCDN_KEYPATH is empty, won't be able to upload releases." >&2
    RELEASE=0
fi

BUILDPATH="$1"

OUTPUTDIR="/tmp/pedigree-release"
if [ ! -z "$2" ]; then
    OUTPUTDIR="$2"
fi

DATESTAMP=`date "+%Y%m%d"`
RELEASE_NAME="pedigree-$DATESTAMP"
if [ ! -z "$3" ]; then
    RELEASE_NAME="$3"
fi

# Get ourselves a temporary directory to work in.
rm -rf $OUTPUTDIR
mkdir -p $OUTPUTDIR

# Bring across the main, minted ISO.
cp "$BUILDPATH"/pedigree.iso $OUTPUTDIR/

# Compress as needed.
echo "Compressing (gzip)..."
gzip --keep --rsyncable $OUTPUTDIR/pedigree.iso
echo "Compressing (bzip2)..."
bzip2 --keep $OUTPUTDIR/pedigree.iso
echo "Compressing (xz)..."
xz --keep $OUTPUTDIR/pedigree.iso

# Now done with the original image.
rm -f $OUTPUTDIR/pedigree.iso

# Final step: turn into release names.
rename "s/pedigree(\..*)*$/$RELEASE_NAME\$1/" $OUTPUTDIR/*

echo "Calculating checksums..."

pushd $OUTPUTDIR
# Checksum all images.
for f in "$RELEASE_NAME"*; do
    md5sum "$f" >$f.md5
    sha1sum "$f" >$f.sha1
    sha256sum "$f" >$f.sha256
done
popd

echo "Release files are now present at $OUTPUTDIR."

if [ $RELEASE -gt 0 ]; then
    echo "Uploading releases..."
    rsync -av --chmod=Du=rwx,Dgo=rx,Fu=rw,Fgo=r -e "ssh -i $KEYCDN_KEYPATH" \
        $OUTPUTDIR/* miselin@rsync.keycdn.com:zones/download/
fi
