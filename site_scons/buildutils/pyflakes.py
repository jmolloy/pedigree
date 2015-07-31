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


import commands
import os

import SCons


def Pyflakes(target, source, env):
    if not (env['pyflakes'] or env['sconspyflakes']):
        return

    # Run pyflakes over .py files, if pyflakes is present.
    pyflakespath = commands.getoutput("which pyflakes")
    if not os.path.exists(pyflakespath):
        return

    for i, pyfile in enumerate(source):
        realpath = pyfile.abspath
        name = os.path.basename(realpath)
        pyflakes = env.Command('pyflakes-%s%d' % (name, i), pyfile, '%s %s' % (pyflakespath, realpath))
        env.AlwaysBuild(pyflakes)


def generate(env):
    action = SCons.Action.Action(Pyflakes)
    builder = env.Builder(action=action, target_factory=env.File,
        source_factory=env.File, single_source=False)

    env.Append(BUILDERS={'Pyflakes': builder})
