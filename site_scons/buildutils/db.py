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
import re
import subprocess
import tempfile

import SCons


def BuildSqliteDB(target, source, env):
    """Generates a sqlite3 DB from the given .sql files."""
    all_sql = ''
    for f in source:
        all_sql += f.get_contents()

    tables = ''
    m = re.findall('^create table .*?;$', all_sql, re.M | re.S | re.I)
    for match in m:
        tables += match + '\n'

    all_sql = re.sub('create table .*?;', '', all_sql, flags=re.M | re.S | re.I)

    with tempfile.NamedTemporaryFile() as f:
        f.write('begin;')
        f.write(tables)
        f.write(all_sql)
        f.write('commit;')
        f.flush()

        f.seek(0)

        subprocess.check_call('sqlite3 %s' % (target[0].abspath), stdin=f, shell=True)


def generate(env):
    action = SCons.Action.Action(BuildSqliteDB)
    builder = env.Builder(action=action, target_factory=env.File,
        source_factory=env.File, single_source=False)

    env.Append(BUILDERS={'Sqlite': builder})
