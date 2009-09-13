#! /usr/bin/env python

import tempfile
import shutil
import os

def doLibc(builddir, inputLibcA, glue_name, ar, cc, libgcc):
    
    print "Building libc..."
    
    tmpdir = tempfile.mkdtemp()
    
    buildOut = builddir + "/libc"

    olddir = os.getcwd()
    os.chdir(tmpdir)

    shutil.copy(inputLibcA, tmpdir + "/libc.a")
    os.system(ar + " x libc.a")

    glue = builddir + "/src/subsys/posix/" + glue_name
    shutil.copy(glue, tmpdir + "/" + glue_name)

    objs_to_remove = ["getpwent", "signal", "fseek", "getcwd", "rename", "rewinddir", "opendir", "readdir", "closedir", "_isatty"]

    for i in objs_to_remove:
        try:
            os.remove("lib_a-" + i + ".o")
        except:
            continue

    os.system(cc + " -nostdlib -shared -Wl,-shared -Wl,-soname,libc.so -L" + libgcc + " -o " + buildOut + ".so *.o -lgcc")
    os.system(ar + " cru " + buildOut + ".a *.o")

    for i in os.listdir("."):
        os.remove(i)

    os.chdir(olddir)
    os.rmdir(tmpdir)

import sys
doLibc(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6])