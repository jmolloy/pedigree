#!/bin/bash

# Fix executable path as compilers are most likely not present in $PATH right now.
export PATH="$SRCDIR/compilers/dir/bin:$PATH"

# Fix include path so newlib's own headers don't override Pedigree's.
CC_VERSION=`$XGCC -dumpversion`
XGCC="$XGCC -nostdinc -nostdlib"
XGCC="$XGCC -I$SRCDIR/compilers/dir/$COMPILER_TARGET/include"
XGCC="$XGCC -I$SRCDIR/compilers/dir/lib/gcc/$COMPILER_TARGET/$CC_VERSION/include"
XGCC="$XGCC -I$SRCDIR/compilers/dir/lib/gcc/$COMPILER_TARGET/$CC_VERSION/include-fixed"

rm -rf mybuild
mkdir -p mybuild
cd mybuild

CC=$XGCC LD=$XLD \
../newlib/configure --host=$COMPILER_TARGET --target=$COMPILER_TARGET \
                    --enable-newlib-multithread --enable-newlib-mb \
                    --disable-newlib-supplied-syscalls \
                    >newlib.log 2>&1

make >>newlib.log 2>&1

cp libc.a $DROPDIR/stock-libc.a
cp libm.a $DROPDIR/stock-libm.a
