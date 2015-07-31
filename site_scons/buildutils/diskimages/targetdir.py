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


def buildImageTargetdir(target, source, env):
    if env['verbose']:
        print '      Copying to ' + os.path.basename(source[0].abspath)
    else:
        print '      Copying to \033[32m' + os.path.basename(source[0].abspath) + '\033[0m'

    builddir = env.Dir(env["PEDIGREE_BUILD_BASE"]).abspath
    imagedir = env.Dir(env['PEDIGREE_IMAGES_DIR']).abspath
    appsdir = env.Dir(env['PEDIGREE_BUILD_APPS']).abspath
    modsdir = env.Dir(env['PEDIGREE_BUILD_MODULES']).abspath
    drvsdir = env.Dir(env['PEDIGREE_BUILD_DRIVERS']).abspath
    libsdir = os.path.join(builddir, 'libs')

    outFile = target[0].path
    targetDir = source[0].path
    source = source[1:]

    # Perhaps the menu.lst should refer to .pedigree-root :)
    os.system("cp -u " + builddir + "/config.db %s/.pedigree-root" % (targetDir,))
    os.system("mkdir -p %s/boot/grub" % (targetDir,))
    os.system("cp -u " + imagedir + "/../grub/menu-hdd.lst %s/boot/grub/menu.lst" % (targetDir,))

    # Copy the kernel, initrd, and configuration database
    for i in source[0:3]:
        os.system("cp -u " + i.abspath + " %s/boot/" % (targetDir,))
    source = source[3:]

    # Create needed directories for missing layout
    os.system("mkdir -p %s/system/modules" % (targetDir,))

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

        os.system("cp -u -R " + i.path + " %s" % (targetDir,) + otherPath)

    os.system("mkdir -p %s/tmp" % (targetDir,))
    os.system("mkdir -p %s/config" % (targetDir,))
    os.system("cp -u " + imagedir + "/../base/config/greeting %s/config/greeting" % (targetDir,))
    os.system("cp -Ru " + imagedir + "/../base/config/term %s/config/term" % (targetDir,))
    os.system("cp -u " + imagedir + "/../base/config/inputrc %s/config/inputrc" % (targetDir,))
    os.system("cp -u " + imagedir + "/../base/.bashrc %s/.bashrc" % (targetDir,))
    os.system("cp -u " + imagedir + "/../base/.profile %s/.profile" % (targetDir,))

    if os.path.exists(outFile):
        os.unlink(outFile)

    with open(outFile, 'w'):
        pass
