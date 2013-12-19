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
    print "Building libc...",
    
    tmpdir = tempfile.mkdtemp()
    
    buildOut = os.path.join(builddir, "libc")

    olddir = os.getcwd()
    os.chdir(tmpdir)

    # Bring across libc.a so we don't modify the actual master copy
    shutil.copy(inputLibcA, os.path.join(tmpdir, "libc.a"))

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
        # "memset",
    ]

    # Remove the object files we manually override in our glue.
    args = [ar, "d", "libc.a"]
    args.extend(['lib_a-%s.o' % (x,) for x in objs_to_remove])
    res = subprocess.call(args, cwd=tmpdir)
    if res:
        print "  (failed -- couldn't remove objects from libc.a)"
        exit(res)

    args = [ar, "x", glue_name]
    res = subprocess.call(args, cwd=tmpdir)
    if res:
        print "  (failed -- couldn't extract glue library)"
        exit(res)

    pedigreec_dir = os.path.dirname(pedigree_c_name)
    glue_dir = os.path.dirname(glue_name)

    cc_cmd = [
        cc,
        "-nostdlib",
        "-shared",
        "-Wl,-soname,libc.so",
        "-o", buildOut + ".so",
        "-L.",
        "-L%s" % (pedigreec_dir,),
        "-L%s" % (glue_dir,),
        "-Wl,--whole-archive",
        "-lc",
        "-lpedigree-glue",
        "-Wl,--no-whole-archive",
        "-lpedigree-c",
        "-lgcc",
    ]

    res = subprocess.call(cc_cmd, cwd=tmpdir)
    if res != 0:
        print "  (failed -- to compile libc.so)"
        exit(res)

    # Freshen libc.a with the glue.
    args = [ar, "r", "libc.a"]
    args.extend([x for x in os.listdir(tmpdir) if x.endswith('.obj')])
    res = subprocess.call(args, cwd=tmpdir)
    if res:
        print "  (failed -- couldn't add glue back to libc.a)"
        exit(res)

    # Copy static library out now.
    shutil.copy(os.path.join(tmpdir, "libc.a"), buildOut + ".a")

    # Clean up.
    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    if not os.uname()[0] == 'Pedigree':
        os.rmdir(tmpdir) # -ENOSYS on Pedigree host. Should fix that.

    print "ok!"

doLibc(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], "")
