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

import tempfile
import shutil
import os

def doLibm(builddir, inputLibmA, ar, cc, libgcc):
    print "Building libm...",
    
    tmpdir = tempfile.mkdtemp()
    
    buildOut = builddir + "/libm"

    olddir = os.getcwd()
    os.chdir(tmpdir)

    shutil.copy(inputLibmA, tmpdir + "/libm.a")
    shutil.copy(inputLibmA, buildOut + '.a')
    os.system(cc + " -nostdlib -shared -Wl,-soname,libm.so -L. -o " + buildOut + ".so -Wl,--whole-archive -lm --Wl,--no-whole-archive -lgcc")

    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    os.rmdir(tmpdir)

    print "ok!"

import sys
doLibm(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], "")
