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
import subprocess
import tarfile

import SCons


def mergeArchives(target, source, env):
  target = target[0]
  removals = env['ARCHIVE_DELETIONS']

  cmd = 'create %s\n' % target.abspath
  for f in source:
    cmd += 'addlib %s\n' % f.abspath
  cmd += 'list\n'
  for r in removals:
    cmd += 'delete %s\n' % r
  cmd += 'save\nend\n'

  proc = subprocess.Popen([env['AR'], '-M'], stdin=subprocess.PIPE,
    stdout=subprocess.PIPE)
  proc.communicate(cmd)

  return proc.returncode


def buildLibc(env, libc_in, glue_in):
  build_dir = env["PEDIGREE_BUILD_BASE"]
  images_dir = env["PEDIGREE_IMAGES_DIR"]
  env["PEDIGREE_IMAGES_DIR"] = env.Dir(images_dir).path

  libc_static_out = env.File(os.path.join(build_dir, "libc.a"))
  libc_static_tmp = env.File(os.path.join(build_dir, "libc_.a"))
  libg_static_out = env.File(os.path.join(build_dir, "libg.a"))
  libg_static_tmp = env.File(os.path.join(build_dir, "libg_.a"))

  libc_shared_out = env.File(os.path.join(build_dir, "libc.so"))
  libg_shared_out = env.File(os.path.join(build_dir, "libg.so"))

  # Bring across libg.a so we don't modify the actual master copy
  libc_static = env.Command(
      libc_static_tmp, libc_in, "$STRIP -g -o $TARGET $SOURCE")

  # Set of object files to remove from the library. These are generally things
  # that Pedigree provides.
  objs_to_remove = [
      "init", "getpwent", "signal", "fseek", "getcwd", "rewinddir", "opendir",
      "readdir", "closedir", "_isatty", "basename", "setjmp", "malloc",
      "mallocr", "calloc", "callocr", "freer", "realloc", "reallocr",
      "malign", "malignr", "mstats", "mtrim", "valloc", "msize",
      "mallinfor", "malloptr", "mallstatsr", "ttyname", "strcasecmp",
      "memset"]

  # What target are we using?
  if env["ON_PEDIGREE"]:
    return
  else:
    if env["ARCH_TARGET"] in ["X64", "HOSTED"]:
      objs_to_remove.extend(["memcpy",])
    elif env["ARCH_TARGET"] in ["ARM"]:
      objs_to_remove.extend(["access",])

  deletions = ["lib_a-%s.o" % (x,) for x in objs_to_remove]

  # Remove the object files we manually override in our glue.
  env['ARCHIVE_DELETIONS'] = deletions
  libg = env.Command(
          libg_static_out, [libc_in, glue_in],
          mergeArchives)
  libc = env.Command(
          libc_static_out, [libc_static_tmp, glue_in],
          mergeArchives)

  # Build the shared object.
  env.Command(
      libg_shared_out, libg_static_out,
      "$LINK -nostdlib -shared -g3 -ggdb -gdwarf-2 -Wl,-soname,libg.so -L. "
      "-L$PEDIGREE_BUILD_BASE -L$PEDIGREE_IMAGES_DIR/libraries -Wl,-Bstatic "
      "-Wl,--whole-archive $SOURCE -lpedigree-c "
      "-Wl,--no-whole-archive -Wl,-Bdynamic -lgcc -lncurses -o $TARGET")
  env.Command(
      libc_shared_out, libc_static_out,
      "$LINK -nostdlib -shared -Wl,-soname,libc.so -L. -L$PEDIGREE_BUILD_BASE"
      "  -L$PEDIGREE_IMAGES_DIR/libraries -Wl,-Bstatic -Wl,--whole-archive "
      "$SOURCE -lpedigree-c -Wl,--no-whole-archive "
      "-Wl,-Bdynamic -lgcc -lncurses -o $TARGET")


def buildLibm(env, libm_in):
  build_dir = env["PEDIGREE_BUILD_BASE"]

  libm_static_in = env.File(os.path.join(build_dir, "stock-libm.a"))
  libm_static_out = env.File(os.path.join(build_dir, "libm.a"))
  libm_shared_out = env.File(os.path.join(build_dir, "libm.so"))

  env.Command(libm_static_out, libm_static_in, "cp $SOURCE $TARGET")
  env.Command(
      libm_shared_out, libm_static_in,
      "$LINK -nostdlib -shared -Wl,-soname,libm.so -L. -o $TARGET -Wl,-Bstatic"
      "  -Wl,--whole-archive $SOURCE -Wl,--no-whole-archive -Wl,-Bdynamic "
      "-lgcc")
