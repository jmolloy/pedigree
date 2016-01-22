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
import struct
import tempfile


def buildImageE2fsprogs(target, source, env):
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

    outFile = target[0].abspath

    # ext2img inserts files into our disk image.
    ext2img = os.path.join(env['HOST_BUILDDIR'], 'ext2img', 'ext2img')

    # TODO(miselin): this image will not have GRUB on it.

    # Copy files into the local images directory, ready for creation.
    shutil.copyfile(os.path.join(builddir, 'config.db'),
                    os.path.join(imagedir, '.pedigree-root'))

    def makedirs(p):
        if not os.path.exists(p):
            os.makedirs(p)

    # Create target directories.
    makedirs(os.path.join(imagedir, 'boot'))
    makedirs(os.path.join(imagedir, 'boot', 'grub'))
    makedirs(os.path.join(imagedir, 'applications'))
    makedirs(os.path.join(imagedir, 'libraries'))
    makedirs(os.path.join(imagedir, 'system/modules'))
    makedirs(os.path.join(imagedir, 'config'))

    # Add GRUB config.
    shutil.copyfile(os.path.join(imagedir, '..', 'grub', 'menu-hdd.lst'),
                    os.path.join(imagedir, 'boot', 'grub', 'menu.lst'))

    # Copy the kernel, initrd, and configuration database
    if env['kernel_on_disk']:
        nth = 3
        if 'STATIC_DRIVERS' in env['CPPDEFINES']:
            nth = 2
        for i in source[0:nth]:
            shutil.copyfile(i.abspath,
                            os.path.join(imagedir, 'boot', i.name))
    else:
        nth = 0
    source = source[nth:]

    # Copy each input file across
    for i in source:
        otherPath = ''
        search, prefix = imagedir, ''

        # Applications
        if appsdir in i.abspath:
            search = appsdir
            prefix = 'applications'

        # Modules
        elif modsdir in i.abspath:
            search = modsdir
            prefix = 'system/modules'

        # Drivers
        elif drvsdir in i.abspath:
            search = drvsdir
            prefix = 'system/modules'

        # User Libraries
        elif libsdir in i.abspath:
            search = libsdir
            prefix = 'libraries'

        # Additional Libraries
        elif builddir in i.abspath:
            search = builddir
            prefix = 'libraries'

        # Already in the image.
        elif imagedir in i.abspath:
            continue

        otherPath = prefix + i.abspath.replace(search, '')

        # Clean out the last directory name if needed
        fn = shutil.copyfile
        if os.path.isdir(i.abspath):
            fn = shutil.copytree

        fn(i.path, os.path.join(imagedir, otherPath))

    # Copy etc bits.
    shutil.copyfile(os.path.join(imagedir, '..', 'base', 'config', 'greeting'),
                    os.path.join(imagedir, 'config', 'greeting'))
    shutil.copyfile(os.path.join(imagedir, '..', 'base', 'config', 'inputrc'),
                    os.path.join(imagedir, 'config', 'inputrc'))
    shutil.copyfile(os.path.join(imagedir, '..', 'base', '.bashrc'),
                    os.path.join(imagedir, '.bashrc'))
    shutil.copyfile(os.path.join(imagedir, '..', 'base', '.profile'),
                    os.path.join(imagedir, '.profile'))

    # Offset into the image for the partition proper to start.
    partition_offset = 0 # 0x10000

    # Build file for creating the disk image.
    base_image = open(outFile, 'w')

    # Create image - 1GiB.
    sz = (1 << 31) + partition_offset
    base_image.truncate(sz)

    # Add a partition table to the front of the image.
    if 0:
        hpc = 16  # Heads per cylinder
        spt = 63  # Sectors per track

        # LBA sector count.
        lba = sz // 512
        end_cyl = lba // (spt * hpc)
        end_head = (lba // spt) % hpc
        end_sector = (lba % spt) + 1

        # Sector start LBA.
        start_lba = partition_offset // 512
        start_cyl = start_lba // (spt * hpc)
        start_head = (start_lba // spt) % hpc
        start_sector = (start_lba % spt) + 1

        # Partition entry.
        entry = '\x80'  # Partition is active/bootable.
        entry += struct.pack('BBB',
                             start_head & 0xFF,
                             start_sector & 0xFF,
                             start_cyl & 0xFF)
        entry += '\x83'  # ext2 partition - ie, a Linux native filesystem.
        entry += struct.pack('BBB',
                             end_head & 0xFF,
                             end_sector & 0xFF,
                             end_cyl & 0xFF)
        entry += struct.pack('=L', start_lba)
        entry += struct.pack('=L', lba)

        # Build partition table.
        partition = '\x00' * 446
        partition += entry
        partition += '\x00' * 48
        partition += '\x55\xAA'
        base_image.write(partition)

    # Generate ext2 filesystem.
    args = [
        env['MKE2FS'],
        '-q',
        '-E', 'root_owner=0:0,offset=%d' % partition_offset,  # Don't use UID/GID from host system.
        '-O', '^dir_index',  # Don't (yet) use directory b-trees.
        '-I', '128',  # Use 128-byte inodes, as grub-legacy can't use bigger.
        '-F',
        '-L',
        'pedigree',
        outFile,
    ]
    subprocess.check_call(args)

    # Populate the image.
    cmdlist = []
    for (dirpath, dirs, files) in os.walk(imagedir):
        target_dirpath = dirpath.replace(imagedir, '')
        if not target_dirpath:
            target_dirpath = '/'

        for d in dirs:
            cmdlist.append('mkdir %s' % (os.path.join(target_dirpath, d),))

        for f in sorted(files):
            source = os.path.join(dirpath, f)
            target = os.path.join(target_dirpath, f)
            if os.path.islink(source):
                link_target = os.readlink(source)
                if link_target.startswith(dirpath):
                    link_target = link_target.replace(dirpath, '').lstrip('/')

                cmdlist.append('symlink %s %s' % (target, link_target))
            elif os.path.isfile(source):
                cmdlist.append('write %s %s' % (source, target))

    base_image.close()

    # Dump our files into the image using ext2img (built as part of the normal
    # Pedigree build, to run on the build system - not on Pedigree).
    with tempfile.NamedTemporaryFile() as f:
        f.write('\n'.join(cmdlist))
        f.flush()

        args = [
            ext2img,
            '-c',
            f.name,
            '-f',
            outFile,
        ]

        subprocess.check_call(args)
