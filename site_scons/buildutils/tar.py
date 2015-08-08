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
import tarfile

import SCons

from buildutils.patcher import Patcher


def Extract(target, source, env):
    with tarfile.open(source[0].abspath, 'r') as f:
        f.extractall(source[0].dir.abspath)

    # Apply patches, if any.
    r = None
    if len(source) > 1:
        r = Patcher(target, source[1:], env)

    return r

def Create(target, source, env):
    args = [env['TAR'], '--transform', 's,.*/,,g', '-czPf', target[0].abspath]
    args.extend([x.abspath for x in source])

    subprocess.check_call(args)
    return None

def generate(env):
    untar_action = SCons.Action.Action(Extract, env.get('TARXCOMSTR'))
    tar_action = SCons.Action.Action(Create, env.get('TARCOMSTR'))

    untar_builder = env.Builder(action=untar_action, target_factory=env.Dir,
        source_factory=env.File)
    tar_builder = env.Builder(action=tar_action, target_factory=env.Dir,
        source_factory=env.File)

    env.Append(BUILDERS={
        'ExtractAndPatchTar': untar_builder,
        'CreateTar': tar_builder})
