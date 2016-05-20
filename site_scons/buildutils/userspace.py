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

import misc


REMOVAL_FLAGS = set(['-nostdinc', '-ffreestanding', '-nostdlib',
                     '-fno-builtin', '-fno-exceptions', '-fno-rtti'])

X86_REMOVAL_FLAGS = set(['-mno-mmx', '-mno-sse'])
X64_REMOVAL_FLAGS = set(['-mno-red-zone', '-mcmodel=kernel'])


# Cleans a set of flags so we can build proper applications rather than
# freestanding binaries.
def fixFlags(env, flags):
    removals = set()
    additions = set()

    removals.update(REMOVAL_FLAGS)

    if env['ARCH_TARGET'] in ['X86', 'X64']:
        removals.update(X86_REMOVAL_FLAGS)
        additions.add('-msse2')
        additions.add('-mfpmath=both')
    if env['ARCH_TARGET'] == 'X64':
        removals.update(X64_REMOVAL_FLAGS)
    if env['ARCH_TARGET'] == 'PPC':
        additions.add('-U__svr4__')
    if env['ARCH_TARGET'] == 'ARM':
        additions.add('-Wno-error=cast-align')

    flags = misc.removeFlags(flags, removals)
    for addition in additions:
        if addition not in flags:
            flags.append(addition)

    return flags
