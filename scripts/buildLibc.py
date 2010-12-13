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

import tempfile
import shutil
import os

def doLibc(builddir, inputLibcA, glue_name, pedigree_c_name, ar, cc, libgcc):
    
    print "Building libc..."
    
    tmpdir = tempfile.mkdtemp()
    
    buildOut = builddir + "/libc"

    olddir = os.getcwd()
    os.chdir(tmpdir)

    shutil.copy(inputLibcA, tmpdir + "/libc.a")
    os.system(ar + " x libc.a")

    glue = glue_name
    shutil.copy(glue, tmpdir + "/" + os.path.basename(glue_name))
    shutil.copy(pedigree_c_name, tmpdir + "/" + os.path.basename(pedigree_c_name))

    objs_to_remove = ["init", "getpwent", "signal", "fseek", "getcwd", "rename", "rewinddir", "opendir", "readdir", "closedir", "_isatty", "basename", "setjmp"]

    for i in objs_to_remove:
        try:
            os.remove("lib_a-" + i + ".o")
        except:
            continue
    
    os.system(ar + " x " + os.path.basename(glue_name))

    os.system(cc + " -nostdlib -shared -Wl,-shared -Wl,-soname,libc.so -o " + buildOut + ".so *.obj *.o -L. -lpedigree-c -lgcc")
    os.system(ar + " cru " + buildOut + ".a *.o")

    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    os.rmdir(tmpdir)

import sys
doLibc(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], "")
