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
import shutil


def buildImageLosetup(target, source, env):
    if env['verbose']:
        print '      Creating ' + os.path.basename(target[0].path)
    else:
        print '      Creating \033[32m' + os.path.basename(target[0].path) + '\033[0m'

    builddir = env.Dir(env["PEDIGREE_BUILD_BASE"]).abspath
    imagedir = env.Dir(env['PEDIGREE_IMAGES_DIR']).abspath
    appsdir = env.Dir(env['PEDIGREE_BUILD_APPS']).abspath
    modsdir = env.Dir(env['PEDIGREE_BUILD_MODULES']).abspath
    drvsdir = env.Dir(env['PEDIGREE_BUILD_DRIVERS']).abspath
    libsdir = os.path.join(builddir, 'libs')

    outFile = target[0].path
    imageBase = source[0].path
    offset = 32256
    source = source[1:]

    # Copy the base image to the destination, overwriting any image that
    # may already exist there.
    if('gz' in imageBase):
        os.system("tar -xzf " + imageBase + " -C .")
        shutil.move(os.path.basename(imageBase).replace('tar.gz', 'img'), outFile)
    else:
        shutil.copy(imageBase, outFile)

    os.mkdir("tmp")
    os.system("sudo mount -o loop,rw,offset=" + str(offset) + " " + outFile + " ./tmp")

    # Perhaps the menu.lst should refer to .pedigree-root :)
    os.system("sudo cp " + builddir + "/config.db ./tmp/.pedigree-root")
    os.system("sudo cp " + imagedir + "/../grub/menu-hdd.lst ./tmp/boot/grub/menu.lst")

    # Copy the kernel, initrd, and configuration database
    nth = 3
    if 'STATIC_DRIVERS' in env['CPPDEFINES']:
        nth = 2
    for i in source[0:nth]:
        os.system("sudo cp " + i.abspath + " ./tmp/boot/")
    source = source[nth:]

    # Copy each input file across
    for i in source:
        otherPath = ''
        search, prefix = imagedir, ''

        # Applications
        if appsdir in i.abspath:
            search = appsdir
            prefix = '/applications'

        # Modules
        elif modsdir in i.abspath:
            search = modsdir
            prefix = '/system/modules'

        # Drivers
        elif drvsdir in i.abspath:
            search = drvsdir
            prefix = '/system/modules'

        # User Libraries
        elif libsdir in i.abspath:
            search = libsdir
            prefix = '/libraries'

        # Additional Libraries
        elif builddir in i.abspath:
            search = builddir
            prefix = '/libraries'

        otherPath = prefix + i.abspath.replace(search, '')

        # Clean out the last directory name if needed
        if(os.path.isdir(i.abspath)):
            otherPath = '/'.join(otherPath.split('/')[:-1])
            if(len(otherPath) == 0 or otherPath[0] != '/'):
                otherPath = '/' + otherPath

        os.system("sudo cp -R " + i.path + " ./tmp" + otherPath)

    os.system("sudo mkdir -p ./tmp/tmp")
    os.system("sudo mkdir -p ./tmp/config")
    os.system("sudo cp " + imagedir + "/../base/config/greeting ./tmp/config/greeting")
    os.system("sudo cp " + imagedir + "/../base/config/inputrc ./tmp/config/inputrc")
    os.system("sudo cp " + imagedir + "/../base/.bashrc ./tmp/.bashrc")
    os.system("sudo cp " + imagedir + "/../base/.profile ./tmp/.profile")
    os.system("sudo umount ./tmp")

    for i in os.listdir("tmp"):
        os.remove(i)
    os.rmdir("tmp")
