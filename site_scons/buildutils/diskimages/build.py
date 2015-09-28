'''
Copyright (c) 2008-2014, Pedigree Developers

Please see the CONTRIB file in the root of the source tree for a full
list of contributors.

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
'''


import os

import debugfs
import losetup
import livecd
import mtools
import targetdir
import post

import SCons


def buildDiskImages(env, config_database):
    builddir = env.Dir(env["PEDIGREE_BUILD_BASE"]).abspath
    imagedir = env.Dir(env['PEDIGREE_IMAGES_DIR']).abspath

    hddimg = os.path.join(builddir, 'hdd.img')
    cdimg = os.path.join(builddir, 'pedigree.iso')

    if (not env['ARCH_TARGET'] in ['X86', 'X64', 'PPC', 'ARM', 'HOSTED'] or
            not (env['distdir'] or not env['nodiskimages'])):
        print 'No hard disk image being built.'
        return

    env.Depends(hddimg, 'libs')

    if env['ARCH_TARGET'] != 'ARM':
        env.Depends(hddimg, 'apps')

    if not 'STATIC_DRIVERS' in env['CPPDEFINES']:
        env.Depends(hddimg, 'initrd')
    env.Depends(hddimg, config_database)

    if not env['nodiskimages'] and not env['noiso']:
        env.Depends(cdimg, hddimg) # Inherent dependency on libs/apps

    fileList = []

    kernel = os.path.join(builddir, 'kernel', 'kernel')
    initrd = os.path.join(builddir, 'initrd.tar')

    apps = os.path.join(builddir, 'apps')
    modules = os.path.join(builddir, 'modules')
    drivers = os.path.join(builddir, 'drivers')

    libc = os.path.join(builddir, 'libc.so')
    libm = os.path.join(builddir, 'libm.so')

    # TODO(miselin): more ARM userspace
    if env['ARCH_TARGET'] != 'ARM':
        libload = os.path.join(builddir, 'libload.so')
        libui = os.path.join(builddir, 'libs', 'libui.so')
    else:
        libload = None
        libui = None

    libpthread = os.path.join(builddir, 'libpthread.so')
    libpedigree = os.path.join(builddir, 'libpedigree.so')
    libpedigree_c = os.path.join(builddir, 'libpedigree-c.so')

    # Build the disk images (whichever are the best choice for this system)
    forcemtools = env['forcemtools']
    buildImage = None
    if (not forcemtools) and env['distdir']:
        fileList.append(env['distdir'])

        buildImage = targetdir.buildImageTargetdir
    elif not forcemtools and (env['MKE2FS'] is not None and env['DEBUGFS'] is not None):
        buildImage = debugfs.buildImageE2fsprogs
    elif not forcemtools and (env['LOSETUP'] is not None):
        fileList += ["#images/hdd_ext2.tar.gz"]

        buildImage = losetup.buildImageLosetup

    if forcemtools or buildImage is None:
        if env.File('#images/hdd_fat32.img').exists():
            fileList += ["#images/hdd_fat32.img"]
        else:
            fileList += ["#images/hdd_fat32.tar.gz"]

        buildImage = mtools.buildImageMtools

    # /boot directory
    if env['kernel_on_disk']:
        if 'STATIC_DRIVERS' in env['CPPDEFINES']:
            fileList += [kernel, config_database]
        else:
            fileList += [kernel, initrd, config_database]

    # Add directories in the images directory.
    for entry in os.listdir(imagedir):
        fileList += [os.path.join(imagedir, entry)]

    # Add applications that we build as part of the build process.
    if os.path.exists(apps):
        for app in os.listdir(apps):
            fileList += [os.path.join(apps, app)]
    else:
        print "Apps directory did not exist at time of build."
        print "'scons' will need to be run again to fully build the disk image."

    # Add modules, and drivers, that we build as part of the build process.
    if os.path.exists(modules):
        for module in os.listdir(modules):
            fileList += [os.path.join(modules, module)]

    if os.path.exists(drivers):
        for driver in os.listdir(drivers):
            fileList += [os.path.join(drivers, driver)]

    # Add libraries
    fileList += [
        libc,
        libm,
        libload,
        libpthread,
        libpedigree,
        libpedigree_c,
        libui,
    ]

    if env['ARCH_TARGET'] in ['X86', 'X64']:
        fileList += [os.path.join(builddir, 'libSDL.so')]

    fileList = [x for x in fileList if x]
    env.Command(hddimg, fileList, SCons.Action.Action(buildImage, None))

    # Build the live CD ISO
    if not env['noiso']:
        env.Command(cdimg, [config_database, initrd, kernel, hddimg],
            SCons.Action.Action(livecd.buildCdImage, None))

    if not env['distdir']:
        post.postImageBuild(hddimg, env)
