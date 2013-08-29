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
import sys
import subprocess

def doLibc(builddir, inputLibcA, glue_name, pedigree_c_name, ar, cc, libgcc):
    
    print "Building libc..."
    
    tmpdir = tempfile.mkdtemp()
    
    buildOut = os.path.join(builddir, "libc")

    olddir = os.getcwd()
    os.chdir(tmpdir)

    shutil.copy(inputLibcA, os.path.join(tmpdir, "libc.a"))
    res = subprocess.call([ar, "x", "libc.a"], cwd=tmpdir)
    if res:
        print "  (failed)"
        exit(res)

    glue = glue_name
    shutil.copy(glue, os.path.join(tmpdir, os.path.basename(glue_name)))
    shutil.copy(pedigree_c_name, os.path.join(tmpdir, os.path.basename(pedigree_c_name)))

    objs_to_remove = [
        "init",
        "getpwent",
        "signal",
        "fseek",
        "getcwd",
        "rename",
        "rewinddir",
        "opendir",
        "readdir",
        "closedir",
        "_isatty",
        "basename",
        "setjmp",
        "malloc",
        "mallocr",
        "calloc",
        "callocr",
        "free",
        "freer",
        "realloc",
        "reallocr",
        "malign",
        "malignr",
        "mallinfor",
        "mstats",
        "mtrim",
        "valloc",
        "msize",
        "mallinfor",
        "malloptr",
        "mallstatsr",
        "ttyname",
        "memcpy",
    ]

    for i in objs_to_remove:
        try:
            os.remove("lib_a-" + i + ".o")
        except:
            continue
    
    res = subprocess.call([ar, "x", os.path.basename(glue_name)], cwd=tmpdir)
    if res != 0:
        print "  (failed)"
        exit(res)

    cc_cmd = [cc, "-nostdlib", "-shared", "-Wl,-shared", "-Wl,-soname,libc.so", "-o", buildOut + ".so", "-L."]
    objs = [x for x in os.listdir(".") if '.obj' in x or '.o' in x]
    cc_cmd.extend(objs)
    cc_cmd.extend(["-lpedigree-c", "-lgcc"])
    res = subprocess.call(cc_cmd, cwd=tmpdir)
    if res != 0:
        print "  (failed)"
        exit(res)

    res = subprocess.call([ar, "cru", buildOut + ".a"] + objs, cwd=tmpdir)
    if res != 0:
        print "  (failed)"
        os.unlink("%s.so" % (buildOut,))
        exit(res)

    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    if not os.uname()[0] == 'Pedigree':
        os.rmdir(tmpdir) # -ENOSYS on Pedigree host. Should fix that.

doLibc(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], "")
