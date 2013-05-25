#! /usr/bin/env python

import tempfile
import shutil
import os

def doLibm(builddir, inputLibmA, ar, cc, libgcc):
    
    print "Building libm..."
    
    tmpdir = tempfile.mkdtemp()
    
    buildOut = builddir + "/libm"

    olddir = os.getcwd()
    os.chdir(tmpdir)

    shutil.copy(inputLibmA, tmpdir + "/libm.a")
    os.system(ar + " x libm.a")
    os.system(cc + " -nostdlib -shared -Wl,-shared -Wl,-soname,libm.so -o " + buildOut + ".so *.o -lgcc")
    os.system(ar + " cru " + buildOut + ".a *.o")

    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    os.rmdir(tmpdir)

import sys
doLibm(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], "") # sys.argv[5])
