#!/bin/bash

# Script that can be run to set up a Pedigree repository for building with minimal
# effort.

set -e

ARM_SCONS_OPTS="arm_beagle=1 armv7=1 arm_cortex_a8=1"

echo "Pedigree Easy Build script"
echo "This script will ask a couple questions and then automatically install"
echo "dependencies and compile Pedigree for you."
echo

old=$(pwd)
script_dir=$(cd -P -- "$(dirname -- "$0")" && pwd -P) && script_dir=$script_dir
cd $old

compiler_build_options=""

real_os=""
if [ ! -e "$script_dir/.easy_os" ]; then

    echo "Checking for dependencies... Which operating system are you running on?"
    echo "Cygwin, Debian/Ubuntu, OpenSuSE, Fedora, OSX, or some other system?"

    if [ $# == 0 ]; then
        read os
    else
        os=$1
        shift
    fi

    shopt -s nocasematch

    real_os=$os

    case "$real_os" in
        debian)
            # TODO: Not sure if the package list is any different for debian vs ubuntu?
            echo "Installing packages with apt-get, please wait..."
            sudo apt-get install libmpfr-dev libmpc-dev libgmp3-dev sqlite3 texinfo scons genisoimage u-boot-tools
            ;;
        ubuntu)
            echo "Installing packages with apt-get, please wait..."
            sudo apt-get install libmpfr-dev libmpc-dev libgmp3-dev sqlite3 texinfo scons genisoimage u-boot-tools
            ;;
        opensuse)
            echo "Installing packages with zypper, please wait..."
            sudo zypper install mpfr-devel mpc-devel gmp3-devel sqlite3 texinfo scons genisoimage
            ;;
        fedora|redhat|centos|rhel)
            echo "Installing packages with YUM, please wait..."
            sudo yum install mpfr-devel gmp-devel libmpc-devel sqlite texinfo scons genisoimage
            ;;
        osx|mac)
            echo "Installing packages with macports, please wait..."
            sudo port install mpfr libmpc gmp libiconv sqlite3 texinfo scons cdrtools wget mtools

            real_os="osx"
            ;;
        cygwin|windows|mingw)
            echo "Please ensure you use Cygwin's 'setup.exe', or some other method, to install the following:"
            echo " - Python"
            echo " - GCC & binutils"
            echo " - libgmp, libmpc, libmpfr"
            echo " - mkisofs/genisoimage"
            echo " - sqlite"
            echo "You will need to find alternative sources for the following:"
            echo " - mtools"
            echo " - scons"

            real_os="cygwin"
            ;;
        *)
            echo "Operating system '$os' is not supported yet."
            echo "You will need to find alternative sources for the following:"
            echo " - Python"
            echo " - GCC & binutils"
            echo " - libgmp, libmpc, libmpfr"
            echo " - mkisofs/genisoimage"
            echo " - sqlite"
            echo " - mtools"
            echo " - scons"
            echo
            echo "If you can modify this script to support '$os', please provide patches."
            ;;
    esac

    shopt -u nocasematch
    
    echo $real_os > "$script_dir/.easy_os"

    echo

else
    real_os=`cat "$script_dir/.easy_os"`

    if [ "$real_os" == "$1" ]; then
        shift
    fi
fi

echo "Please wait, checking for a working cross-compiler."
echo "If none is found, the source code for one will be downloaded, and it will be"
echo "compiled for you."

# Special parameters for some operating systems when building cross-compilers
case $real_os in
    osx)
        compiler_build_options="$compiler_build_options osx-compat"
        ;;
esac

# Install cross-compilers
"$script_dir/scripts/checkBuildSystemNoInteractive.pl" arm-pedigree "$script_dir/pedigree-compiler" $compiler_build_options

old=$(pwd)
cd "$script_dir"

set +e

# Update the local working copy only if it is clean.
changed=`git status -s -uno`
if [ -z "$changed" ]; then
    git pull --rebase > /dev/null 2>&1
fi

set -e

# Run a quick build of libc and libm for the rest of the build system.
scons CROSS=$script_dir/compilers/dir/bin/arm-pedigree- $ARM_SCONS_OPTS build/libc.so build/libm.so

# Pull down libtool.
echo
echo "Configuring the Pedigree UPdater..."

$script_dir/setup_pup.py arm

# TODO: build some ARM packages...

# Build GCC again with access to the newly built libc.
# This will create a libstdc++ that can be used by pedigree-apps to build GCC
# again, this time with a shared libstdc++. pedigree-apps should then build GCC
# again to build it against the shared libstdc++. Once a working shared
# libstdc++ exists, the static one built here is no longer relevant.
# What a mess!
$script_dir/scripts/checkBuildSystemNoInteractive.pl arm-pedigree $script_dir/pedigree-compiler $compiler_build_options "libcpp"

set +e

echo
echo "Ensuring CDI is up-to-date."

# Setup all submodules, make sure they are up-to-date
git submodule init > /dev/null 2>&1
git submodule update > /dev/null 2>&1

echo
echo "Installing a base set of packages..."

# TODO: build some ARM packages...

set -e

echo
echo "Beginning the Pedigree build."
echo

# Build Pedigree.
scons CROSS="$script_dir/compilers/dir/bin/arm-pedigree-" $ARM_SCONS_OPTS $*

cd "$old"

echo
echo
echo "Pedigree is now ready to be built without running this script."
echo "To build in future, run the following command in the '$script_dir' directory:"
echo "scons"
echo
echo "If you wish, you can continue to run this script. It won't ask questions"
echo "anymore, unless you remove the '.easy_os' file in '$script_dir'."
echo
echo "You can also run scons --help for more information about options."
echo
echo "Patches should be posted in the issue tracker at http://pedigree-project.org/projects/pedigree/issues"
echo "Support can be found in #pedigree on irc.freenode.net."
echo
echo "Have fun with Pedigree! :)"

