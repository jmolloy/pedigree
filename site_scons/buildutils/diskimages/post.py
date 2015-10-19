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


def postImageBuild(img, env, iso=False):
    if env['QEMU_IMG'] is None:
        return

    builddir = env.Dir(env["PEDIGREE_BUILD_BASE"]).abspath

    additional_images = {}
    if not iso:
        if env['createvdi'] or env['createvmdk']:
            additional_images = {
                'vdi': env.File(os.path.join(builddir, 'hdd.vdi')),
                'vmdk' : env.File(os.path.join(builddir, 'hdd.vmdk')),
            }

        if env['createvdi'] and 'vdi' in additional_images:
            target = additional_images['vdi'].path
            env.Command(target, img,
                '$QEMU_IMG convert -O vpc $SOURCE $TARGET')

        if env['createvmdk'] and 'vmdk' in additional_images:
            target = additional_images['vmdk'].path
            env.Command(target, img,
                '$QEMU_IMG convert -f raw -O vmdk $SOURCE $TARGET')

    # Create hash files.
    sha = '256'
    md5sum = env.Detect('md5sum')
    shasum = env.Detect('sha%ssum' % sha)
    if md5sum:
        env.Command('%s.md5' % img, img, '%s -b $SOURCE >$TARGET' % md5sum)
    if shasum:
        env.Command('%s.sha%s' % (img, sha), img, '%s $SOURCE >$TARGET' % shasum)
