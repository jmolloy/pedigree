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


def buildCdImage(target, source, env):
    if env['verbose']:
        print '      Creating ' + os.path.basename(target[0].path)
    else:
        print '      Creating \033[32m' + os.path.basename(target[0].path) + '\033[0m'

    pathToGrub = env.Dir('#images/grub').abspath

    # Select correct stage2_eltorito for the target.
    stage2_eltorito = 'stage2_eltorito-' + env['ARCH_TARGET'].lower()

    # mkisofs modifies stage2_eltorito to do its work.
    pathToStage2 = os.path.join(pathToGrub, stage2_eltorito)
    shutil.copy(pathToStage2, '%s.mkisofs' % pathToStage2)
    pathToStage2 += '.mkisofs'

    args = [
        env['MKISOFS'],
        '-D',
        '-joliet',
        '-graft-points',
        '-quiet',
        '-input-charset',
        'iso8859-1',
        '-R',
        '-b',
        'boot/grub/stage2_eltorito',
        '-no-emul-boot',
        '-boot-load-size',
        '4',
        '-boot-info-table',
        '-o',
        target[0].path,
        '-V',
        'PEDIGREE',
        'boot/grub/stage2_eltorito=%s' % (pathToStage2,),
        'boot/grub/menu.lst=%s' % (os.path.join(pathToGrub, 'menu.lst'),),
        'boot/kernel=%s' % (source[2].abspath,),
        'boot/initrd.tar=%s' % (source[1].abspath,),
        #'/livedisk.img=%s' % (source[3].abspath,),
        '.pedigree-root=%s' % (source[0].abspath,),
        #'config.db=%s' % (source[0].abspath,),
    ]
    result = subprocess.call(args)

    os.unlink(pathToStage2)

    return result
