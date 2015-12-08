# The Pedigree Operating System

[![Build Status](https://travis-ci.org/miselin/pedigree.svg?branch=develop)](https://travis-ci.org/miselin/pedigree)

## Downloads

* [Latest ISO](https://dl.pedigree-project.org/pedigree-latest.iso.gz)
    [SHA256](https://dl.pedigree-project.org/pedigree-latest.iso.gz.sha256)
* [Nightly ISO](https://dl.pedigree-project.org/pedigree-nightly.iso.gz)
    [SHA256](https://dl.pedigree-project.org/pedigree-nightly.iso.gz.sha256)
* [All Downloads](https://dl.pedigree-project.org)

The latest disk image is the most recent successful build of Pedigree from our
[Buildbot](http://build.pedigree-project.org). There are no guarantees of
stability or even functionality of these builds.

The nightly disk image is from nightly builds on our Buildbot, with the same
disclaimer.

## Build Dependencies

You'll need at least the following to build Pedigree and its compilers:

* SCons (>1.2.0)
* `libmpfr`, `libgmp`, and `libmpc` headers (typically via `-dev` packages)
* SQLite3
* genisoimage and/or mkisofs
* perl
* autoconf

## Building Pedigree with Easy Build

We highly recommend you first try one of our Easy Build scripts before you try
and run SCons manually. There's a little bit of work involved in setting up a
build of Pedigree for the first time, which the Easy Build script handles for
you. After that it's as easy as just running `scons` at the command line.

Just run `./easy_build_[target].sh` to build Pedigree. Valid options for
`target` include:

* x64
* arm
* hosted (for a version of the kernel that runs on Linux)

Dependencies and a cross-compiler will be installed and/or created, allowing
you to jump straight into testing Pedigree.

To build Pedigree at any point after this, just run `scons`. The build system
remembers the configuration the Easy Build specified for you.

### Different Targets

To switch between architectures, just remove `options.cache` and
`.autogen.cache`, and then run an Easy Build script.

## Building Pedigree Manually

Alternatively, you can build manually.

### Step 1: Cross-Compiler

To build a cross-compiler, in the root of the Pedigree tree, run:

`$ ./scripts/checkBuildSystemNoInteractive.pl $TARGET-pedigree \
    $PWD/pedigree-compiler`

If you are building on OSX, you should also pass `osx-compat` as the final
parameter to the script.

Valid targets include:

* `x86_64`
* `armv7`

### Step 2: Pedigree Base

Configure the Pedigree UPdater (pup) to start:

`$ ./setup_pup.py amd64  # (or arm) && ./run_pup.sh sync`

You'll need at least Pedigree's `libtool` to continue:

`$ ./run_pup.sh install libtool`

Now, build an initial `libc` and `libm`:

`$ scons CROSS=$PWD/pedigree-compiler/bin/$TARGET-pedigree- build/libc.so \
    build/libm.so`

With this complete, the compiler build process can be completed:

`$ ./scripts/checkBuildSystemNoInteractive.pl $TARGET-pedigree \
    $PWD/pedigree-compiler libcpp`

### Step 3: Required Packages

Install necessary packages to build the full userspace:

```
$ ./run_pup.py install libpng
$ ./run_pup.py install libfreetype
$ ./run_pup.py install libiconv
$ ./run_pup.py install zlib
$ ./run_pup.py install bash
$ ./run_pup.py install coreutils
$ ./run_pup.py install fontconfig
$ ./run_pup.py install pixman
$ ./run_pup.py install cairo
$ ./run_pup.py install expat
$ ./run_pup.py install mesa
$ ./run_pup.py install ncurses
$ ./run_pup.py install gettext
$ ./run_pup.py install pango
$ ./run_pup.py install glib
$ ./run_pup.py install harfbuzz
$ ./run_pup.py install libffi
$ ./run_pup.py install gcc
```

### Step 4: Final Build

Finally, build the rest of the kernel and userspace:

`$ scons`

From now on, you can simply run `scons` to build Pedigree.

## Running Pedigree

Boot from `build/pedigree.iso`, with an attached disk for `build/hdd.img`, to
run Pedigree.

You can also specify `createvmdk=1` and/or `createvdi=1` to create VMDK or VDI
disk images for your emulator. These options require `qemu-img`.

## Images Directory

The images/local directory allows you to use `pup`, Pedigree's package manager,
to manage your hard disk image file set. If you ran the Easy Build script, pup
is already configured and ready to go.

Simply run:

`$ ./run_pup.sh sync`

to synchronise your local pup repository with the server.

Then you can run:
`$ ./run_pup.sh install <package>`
to install a package.

Visit http://pup.pedigree-project.org to see a list of all packages that are
available and can be downloaded.

Remember to re-run `scons` after installing a package to ensure your disk image
has the new package on it. You may need to `rm build/hdd.img` if SCons doesn't
detect that the images directory has changed.

You can also add arbitrary files to the images/local directory to use them at
runtime. For example, you could create a directory under `users` for yourself,
and add a `.bashrc` and `.vimrc`.

## User Management

A utility script, `scripts/manage_users.py`, is provided to add or remove users
from the database for use at runtime.

## Reporting Issues

Report any issues on the project tracker at http://pedigree-project.org

## Contact

You can find us in #pedigree on Freenode IRC.

## Contributing

We welcome contributions. The preferred mechanism for contributing is via pull
requests. See the issue trackers at http://pedigree-project.org if you need
ideas. Alternatively, come join us in our IRC channel on Freenode (see above).

We highly recommend working through a successful build and playing with some of
Pedigree's features in a VM before leaping into contributing. This will help
with understanding much of what you see in the code, and also potentially give
you some more ideas about areas to contribute to.
