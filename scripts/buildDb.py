#! /usr/bin/env python
# coding=utf8

#
# Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

import os
import re
import tempfile
import subprocess

print "Building configuration database..."

startdir = './src/'
sql = ''
tables = ''

def walk(dummy, dirr, files):
    for child in files:
        if child == 'schema':
            global sql
            sql += open(dirr + '/' + child).read()

os.path.walk(startdir, walk, 'null')

m = re.findall('^create table .*?;$', sql, re.M|re.S|re.I)
for match in m:
    tables += match + '\n'

# Gay -- can't use flags here so can't use case-insensitive or
# dot-matches-all search.
sql = re.sub('create table (.|\n)*?;', '', sql)
sql = re.sub('CREATE TABLE (.|\n)*?;', '', sql)

p = subprocess.Popen(['sqlite3', './build/config.db'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
stdoutdata, stderrdata = p.communicate('begin;'+tables+sql+'commit;')
if stdoutdata:
    print stdoutdata
