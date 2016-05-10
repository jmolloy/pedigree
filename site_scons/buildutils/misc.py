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

import buildutils.db
import buildutils.downloader
import buildutils.header
import buildutils.patcher
import buildutils.pyflakes
import buildutils.tar


def generate(env):
    buildutils.db.generate(env)
    buildutils.downloader.generate(env)
    buildutils.header.generate(env)
    buildutils.patcher.generate(env)
    buildutils.pyflakes.generate(env)
    buildutils.tar.generate(env)


def safeAppend(a, b):
    """Safely append flags from b to a."""
    a = str(a).split()
    b = str(b).split()
    for item in b:
        if not item in a:
            a.append(item)
    return ' '.join(a)


def prettifyBuildMessages(env):
    if not env['verbose']:
        if env['nocolour'] or os.environ.get('TERM') == 'dumb' or os.environ.get('TERM') is None:
            env['CCCOMSTR']   =    '     Compiling $TARGET'
            env['CXXCOMSTR']  =    '     Compiling $TARGET'
            env['SHCCCOMSTR'] =    '     Compiling $TARGET (shared)'
            env['SHCXXCOMSTR']=    '     Compiling $TARGET (shared)'
            env['ASCOMSTR']   =    '    Assembling $TARGET'
            env['LINKCOMSTR'] =    '       Linking $TARGET'
            env['SHLINKCOMSTR'] =  '       Linking $TARGET'
            env['ARCOMSTR']   =    '     Archiving $TARGET'
            env['RANLIBCOMSTR'] =  '      Indexing $TARGET'
            env['NMCOMSTR']   =    '  Creating map $TARGET'
            env['DOCCOMSTR']  =    '   Documenting $TARGET'
            env['TARCOMSTR']  =    '      Creating $TARGET'
            env['TARXCOMSTR'] =    '    Extracting $SOURCE'
            env['DLCOMSTR']   =    '   Downloading $TARGET'
            env['PTCOMSTR']   =    '      Patching $TARGET'
        else:
            env['CCCOMSTR']   =    '     Compiling \033[32m$TARGET\033[0m'
            env['CXXCOMSTR']  =    '     Compiling \033[32m$TARGET\033[0m'
            env['SHCCCOMSTR'] =    '     Compiling \033[32m$TARGET\033[0m (shared)'
            env['SHCXXCOMSTR']=    '     Compiling \033[32m$TARGET\033[0m (shared)'
            env['ASCOMSTR']   =    '    Assembling \033[32m$TARGET\033[0m'
            env['LINKCOMSTR'] =    '       Linking \033[32m$TARGET\033[0m'
            env['SHLINKCOMSTR'] =  '       Linking \033[32m$TARGET\033[0m'
            env['ARCOMSTR']   =    '     Archiving \033[32m$TARGET\033[0m'
            env['RANLIBCOMSTR'] =  '      Indexing \033[32m$TARGET\033[0m'
            env['NMCOMSTR']   =    '  Creating map \033[32m$TARGET\033[0m'
            env['DOCCOMSTR']  =    '   Documenting \033[32m$TARGET\033[0m'
            env['TARCOMSTR']  =    '      Creating \033[32m$TARGET\033[0m'
            env['TARXCOMSTR'] =    '    Extracting \033[32m$SOURCE\033[0m'
            env['DLCOMSTR']   =    '   Downloading \033[32m$TARGET\033[0m'
            env['PTCOMSTR']   =    '      Patching \033[32m$TARGET\033[0m'


def removeFlags(flags, removal):
    """Safely removes the given set of removal words from the given flags.

    It's not possible to simply use set operations as this can remove flags
    that are actually appropriately duplicated within a command line.

    Args:
        flags: flags to remove words from
        removal: iterable of words to remove

    Returns:
        Updated flags.
    """
    if isinstance(flags, str):
        flags = flags.split(' ')
    if isinstance(removal, str):
        removal = removal.split(' ')

    for flag in removal:
        try:
            flags.remove(flag)
        except ValueError:
            pass  # No error.

    return flags


def removeFromAllFlags(env, removal):
    """Remove words from all compilation flags in the given Environment."""
    for key in ('CFLAGS', 'CCFLAGS', 'CXXFLAGS'):
        env[key] = buildutils.misc.removeFlags(env[key], removal)
        if ('TARGET_%s' % key) in env:
            env['TARGET_%s' % key] = buildutils.misc.removeFlags(
                env['TARGET_%s' % key], removal)


def stubSuffix(env, dash=True):
    suffix = 'noarch'
    if env['ARCH_TARGET'] == 'X86':
        suffix = 'i686'
    elif env['ARCH_TARGET'] == 'X64':
        suffix = 'amd64'
    elif env['ARCH_TARGET'] == 'PPC':
        suffix = 'ppc'
    elif env['ARCH_TARGET'] == 'ARM':
        suffix = 'arm'
    elif env['ARCH_TARGET'] == 'HOSTED':
        suffix = 'hosted'

    if dash:
        suffix = '-%s' % suffix
    return suffix
