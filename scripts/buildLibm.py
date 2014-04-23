#! /usr/bin/env python

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
    os.system(cc + " -nostdlib -shared -Wl,-soname,libm.so -L. -o " + buildOut + ".so -Wl,--whole-archive -lm -Wl,--no-whole-archive -lgcc")

    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    os.rmdir(tmpdir)

    print "ok!"

import sys
doLibm(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], "")
