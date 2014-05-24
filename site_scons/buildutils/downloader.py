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
import urllib2

import SCons


def UrlDownloader(target, source, env):
    assert len(source) == 1
    assert len(target) == 1

    # Already exists, don't re-open target.
    if os.path.exists(target[0].abspath):
        return None

    stream = urllib2.urlopen(str(source[0]))
    with open(target[0].abspath, "wb") as f:
        shutil.copyfileobj(stream, f)
    stream.close()

    return None


def generate(env):
    action = SCons.Action.Action(UrlDownloader, env.get('DLCOMSTR'))
    builder = env.Builder(action=action, target_factory=env.File,
        source_factory=env.Value, single_source=1)

    env.Append(BUILDERS={'Download': builder})
