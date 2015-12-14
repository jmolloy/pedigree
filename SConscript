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

# vim: set filetype=python:

import os
import SCons

from buildutils import fs, db
from buildutils.diskimages import build

Import(['env', 'userspace_env'])

if env['build_modules']:
  # POSIX subsystem.
  SConscript(os.path.join('src', 'subsys', 'posix', 'SConscript'),
             exports=['env', 'userspace_env'])
  # Pedigree-C subsystem.
  SConscript(os.path.join('src', 'subsys', 'pedigree-c', 'SConscript'),
             exports=['env', 'userspace_env'])
  # Native subsystem.
  if 'NATIVE_SUBSYSTEM' in env['EXTRA_CONFIG']:
      SConscript(os.path.join('src', 'subsys', 'native', 'SConscript'),
                 exports=['env', 'userspace_env'])
  # Kernel drivers and modules.
  SConscript(os.path.join('src', 'modules', 'SConscript'), exports=['env'])

if env['build_kernel']:
  SConscript(os.path.join('src', 'system', 'kernel', 'SConscript'), exports=['env'])
  if env['BOOT_DIR']:
    SConscript(os.path.join('src', 'system', 'boot', env['BOOT_DIR'], 'SConscript'), exports=['env'])

# On X86, X64 and PPC we build applications and LGPL libraries
if env['build_apps']:
  SConscript(os.path.join('src', 'user', 'SConscript'),
             exports=['env', 'userspace_env'])
if env['build_lgpl']:
  SConscript(os.path.join('src', 'lgpl', 'SConscript'),
             exports=['env', 'userspace_env'])

# Build the configuration database (no dependencies)
config_database = os.path.join(env["PEDIGREE_BUILD_BASE"], 'config.db')
if env['build_configdb']:
  configSchemas = fs.find_files(env.Dir('#src').abspath, lambda x: x == 'schema', [])
  env.Sqlite(config_database, configSchemas)

  if 'STATIC_DRIVERS' in env['CPPDEFINES']:
      # Generate config database header file for static inclusion.
      config_header = env.File('#src/modules/system/config/config_database.h')
      env.FileAsHeader(config_header, config_database)

# Build disk images.
if env['build_images']:
  build.buildDiskImages(env, config_database)

# Pyflakes - linting python files in the tree.
rootdir = env.Dir("#").abspath
imagesroot = env.Dir("#images").abspath

if env['pyflakes'] or env['sconspyflakes']:
    # Find .py files in the tree, excluding the images directory.
    pyfiles = fs.find_files(rootdir, lambda x: x.endswith('.py'), [imagesroot])
    pyflakes = env.Pyflakes('pyflakes-result', pyfiles)
    env.AlwaysBuild(pyflakes)
