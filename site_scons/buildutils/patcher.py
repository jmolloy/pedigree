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

import subprocess

import SCons


def Patcher(target, source, env):
    patchbin = env.get('PATCH', ['patch'])
    patchopts = env.get('PATCHOPTS', ['-p1'])

    args = patchbin + patchopts
    args.extend(['-i', source[0].abspath])

    subprocess.check_call(args, cwd=target[0].abspath)

def generate(env):
    action = SCons.Action.Action(Patcher, env.get('PTCOMSTR'))
    builder = env.Builder(action=action, target_factory=env.Dir,
        source_factory=env.File)

    env.Append(BUILDERS={'Patch': builder})
