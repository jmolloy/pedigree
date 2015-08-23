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
import subprocess
import tempfile


def buildImageMtools(target, source, env):
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

    pathToGrub = env.Dir("#images/grub").abspath

    outFile = target[0].path
    imageBase = source[0].path
    source = source[1:]

    destDrive = "C:"

    execenv = os.environ.copy()
    execenv['MTOOLS_SKIP_CHECK'] = '1'

    def domkdir(name):
        args = [
            'mmd',
            '-Do',
            '%s%s' % (destDrive, name),
        ]

        subprocess.check_call(args, stdout=subprocess.PIPE, env=execenv)

    def docopy(source, dest):
        args = [
            'mcopy',
            '-Do',
        ]

        # Multiple sources to the same destination directory
        if isinstance(source, list):
            args.extend(source)

        # Recursively copy directories
        elif os.path.isdir(source):
            args.extend(['-bms', source])

        # Single, boring source.
        else:
            args.append(source)

        args.append('%s%s' % (destDrive, dest))

        # Some of these copies may fail due to missing symlinks etc
        subprocess.call(args, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, env=execenv)

        sources = source
        if not isinstance(source, list):
            sources = [source]

        filelist = []
        for item in sources:
            if os.path.isdir(item):
                for (p, _, n) in os.walk(item):
                    filelist.extend([os.path.join(p, x) for x in n])
            else:
                filelist.append(item)

        for path in filelist:
            if os.path.islink(path):
                realpath = os.path.relpath(path, imagedir)
                target = os.readlink(path)
                common = os.path.commonprefix([target, imagedir])
                if common == imagedir:
                    target = os.path.relpath(target, imagedir)
                    target = '/%s' % (target,)

                # Delete old file from the image.
                args = ['mdel', os.path.join(destDrive, realpath)]
                subprocess.call(args, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, env=execenv)

                # Create symlink.
                with tempfile.NamedTemporaryFile() as f:
                    f.write(target)
                    f.flush()

                    args = ['mcopy', '-Do', f.name,
                            os.path.join(destDrive, '%s.__sym' % (realpath,))]
                    subprocess.call(args, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, env=execenv)

    # Copy the base image to the destination, overwriting any image that
    # may already exist there.
    if('gz' in imageBase):
        args = ['tar', '-xzf', imageBase, '-C', os.path.dirname(outFile)]
        result = subprocess.call(args)
        if result != 0:
            return result

        outbasename = os.path.basename(outFile)
        actualbasename = os.path.basename(imageBase).replace('tar.gz', 'img')

        # Caveat where build/ is on a different filesystem (why would you do this)
        if outbasename != actualbasename:
            os.rename(
                os.path.join(builddir, actualbasename),
                outFile
            )
    else:
        shutil.copy(imageBase, outFile)

    # Calculate the full set of operations we need to do, before running commands.

    mkdirops = [
        '/config',
        '/system',
        '/system/modules',
    ]

    nth = 3
    if 'STATIC_DRIVERS' in env['CPPDEFINES']:
        nth = 2

    copyops = [
        ([x.abspath for x in source[0:nth]], '/boot'),
        (os.path.join(builddir, 'config.db'), '/.pedigree-root'),
        (os.path.join(pathToGrub, 'menu-hdd.lst'), '/boot/grub/menu.lst'),
        (os.path.join(imagedir, '..', 'base', 'config', 'greeting'), '/config/greeting'),
        (os.path.join(imagedir, '..', 'base', 'config', 'inputrc'), '/config/inputrc'),
        (os.path.join(imagedir, '..', 'base', '.bashrc'), '/.bashrc'),
        (os.path.join(imagedir, '..', 'base', '.profile'), '/.profile'),
    ]

    for i in source[nth:]:
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

        dirname = os.path.dirname(otherPath)
        if not dirname.endswith('/'):
            dirname += '/'

        copyops.append((i.abspath, dirname))

    try:
        # Open for use in mtools
        mtsetup = env.File("#/scripts/mtsetup.sh").abspath
        subprocess.check_call([mtsetup, '-h', outFile, "1"], stdout=subprocess.PIPE)

        for name in mkdirops:
            # print "mkdir %s" % (name,)
            domkdir(name)

        for src, dst in copyops:
            # print "cp %s -> %s" % (src, dst)
            docopy(src, dst)
    except subprocess.CalledProcessError as e:
        os.unlink(outFile)
        return e.returncode

    return 0
