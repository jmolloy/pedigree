#!/usr/bin/env python2.7
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

import commands
import getpass
import os
import re
import SCons
import sys

from socket import gethostname
from datetime import datetime

from buildutils import misc
from defaults import *

opts = Variables('options.cache')
opts.AddVariables(
    ('CROSS', 'Base for cross-compilers, tool names will be appended automatically.', ''),
    ('CC', 'Sets the C compiler to use.'),
    ('CXX', 'Sets the C++ compiler to use.'),
    ('AS', 'Sets the assembler to use.'),
    ('AR', 'Sets the archiver to use.'),
    ('LINK', 'Sets the linker to use.'),
    ('STRIP', 'Path to the `strip\' executable.'),
    ('OBJCOPY', 'Path to `objcopy\' executable.'),
    ('CFLAGS', 'Sets the C compiler flags.', ''),
    ('CCFLAGS', 'Sets the C/C++ generic compiler flags.', ''),
    ('CXXFLAGS', 'Sets the C++ compiler flags.', ''),
    ('ASFLAGS', 'Sets the assembler flags.', ''),
    ('LINKFLAGS', 'Sets the linker flags', ''),
    ('BUILDDIR', 'Directory to place build files in.','build'),

    BoolVariable('cripple_hdd','Disable writing to hard disks at runtime.',1),
    BoolVariable('debugger','Whether or not to enable the kernel debugger.',1),
    BoolVariable('debug_logging','Whether to enable debug-level logging, which can dump massive amounts of data to the kernel log. Probable performance hit too, use at your own risk.',1),
    BoolVariable('superdebug', 'Super debug is like the debug_logging option, except even MORE verbose. Expect hundreds of thousands of lines of output to the kernel log.', 0),
    
    BoolVariable('usb_verbose_debug','When set, USB low-level drivers will dump massive amounts of debug information to the log. When not set, only layers such as mass storage will.',0),

    BoolVariable('verbose','Display verbose build output.',0),
    BoolVariable('nocolour','Don\'t use colours in build output.',0),
    BoolVariable('verbose_link','Display verbose linker output.',0),
    BoolVariable('warnings', 'If nonzero, compilation without -Werror', 1),
    BoolVariable('installer', 'Build the installer', 0),
    BoolVariable('genflags', 'Whether or not to generate flags and things dynamically.', 1),
    BoolVariable('ccache', 'Prepend ccache to cross-compiler paths (for use with CROSS)', 0),
    BoolVariable('distcc', 'Prepend distcc to cross-compiler paths (for use with CROSS)', 0),
    BoolVariable('pyflakes', 'Set to one to run pyflakes over Python scripts in the tree', 0),
    BoolVariable('sconspyflakes', 'Set to one to run pyflakes over SConstruct/SConscripts in the tree', 0),
    BoolVariable('travis', 'Set to one/true to indicate that this is a build on Travis-CI.', 0),
    ('iwyu', 'If set, use the given as a the C++ compiler for include-what-you-use. Use -i with scons if you use IWYU.', ''),
    
    BoolVariable('cache', 'Cache SCons options across builds (highly recommended).', 1),
    BoolVariable('genversion', 'Whether or not to regenerate Version.cc if it already exists.', 1),
    
    ('distdir', 'Directory to install a Pedigree directory structure to, instead of a disk image. Empty will use disk images.', ''),
    BoolVariable('forcemtools', 'Force use of mtools (and the FAT filesystem) even if losetup is available.', 1),
    BoolVariable('createvdi', 'Convert the created hard disk image to a VDI file for VirtualBox after it is created.', 0),
    BoolVariable('createvmdk', 'Convert the created hard disk image to a VMDK file for VMware after it is created.', 0),
    BoolVariable('nodiskimages', 'Whether or not to avoid building disk images for distribution.', 0),
    BoolVariable('noiso', 'Whether or not to avoid building an ISO.', 0),
    
    BoolVariable('pup', 'If 1, you are managing your images/local directory with the Pedigree UPdater (pup) and want that instead of the images/<arch> directory.', 1),
    
    BoolVariable('serial_is_file', 'If 1, the serial port is connected to a file (ie, an emulated serial port). If zero, the serial port is connected to a VT100-compatible terminal.', 1),
    BoolVariable('ipv4_forwarding', 'If 1, enable IPv4 forwarding.', '0'),
    
    BoolVariable('enable_ctrlc', 'If 1, the ability to use CTRL-C to kill running tasks is enabled.', 1),
    BoolVariable('multiple_consoles', 'If 1, the TUI is built with the ability to create and move between multiple virtual consoles.', 1),
    BoolVariable('memory_log', 'If 1, memory logging on the second serial line is enabled.', 1),
    BoolVariable('memory_log_inline', 'If 1, memory logging will be output alongside conventional serial output.', 0),
    BoolVariable('memory_tracing', 'If 1, trace memory allocations and frees (for statistics and for leak detection) on the second serial line. EXCEPTIONALLY SLOW.', 0),
    
    BoolVariable('multiprocessor', 'If 1, multiprocessor support is compiled in to the kernel.', 0),
    BoolVariable('apic', 'If 1, APIC support will be built in (not to be confused with ACPI).', 0),
    BoolVariable('acpi', 'If 1, ACPI support will be built in (not to be confused with APIC).', 0),
    BoolVariable('smp', 'If 1, SMP support will be built in.', 0),
    
    BoolVariable('nogfx', 'If 1, the standard 80x25 text mode will be used. Will not load userspace if set to 1.', 0),

    # PC architecture options.
    BoolVariable('mach_pc', 'Target a typical PC architecture.', 0),

    # ARM options
    BoolVariable('arm_integrator', 'Target the Integrator/CP development board', 0),
    BoolVariable('arm_versatile', 'Target the Versatile/PB development board', 0),
    BoolVariable('arm_beagle', 'Target the BeagleBoard', 0),

    BoolVariable('arm_9', 'Target the ARM9 processor family (currently only ARM926E)', 0),
    BoolVariable('armv7', 'Target the ARMv7 architecture family', 0),

    BoolVariable('arm_cortex_a8', 'Tune and optimise for the ARM Cortex A8 (use in conjunction with the `armv7\' option.', 0),

    BoolVariable('arm_bigendian', 'Is this ARM target big-endian?', 0),

    BoolVariable('linux', 'Is this build to run on Linux?', 0),
    
    ('uimage_target', 'Where to copy the generated uImage.bin file to.', '~'),
)

# Autogenerated options that should never be edited by hand.
autogen_opts = Variables('.autogen.cache')
autogen_opts.AddVariables(
    ('COMPILER_TARGET', 'Compiler target (eg, i686-pedigree).', None),
    ('COMPILER_VERSION', 'Compiler version (eg, 4.5.1).', None),
    ('CPPDEFINES', 'Final set of preprocessor definitions.', None),
    ('ARCH_TARGET', 'Automatically generated architecture name.', None),
    ('HOST_PLATFORM', 'Platform for the compile host.', None),
    BoolVariable('ON_PEDIGREE', 'Whether we are on Pedigree or not.', False),
    BoolVariable('reset_flags', 'Whether to reset *FLAGS variables or not. '
        'Avoid using where possible. Toggles to False after use.', True),
)

# Load the autogenerated environment first up.
autogen_env = Environment(options=autogen_opts, tools=[])

# Make sure we're running on a sane SCons version (this actually builds
# an environment, as it happens).
EnsureSConsVersion(0, 98, 0)

# If we need to reset flags, we do that now, before loading the main
# environment.
# This is because if we need to reset flags to actually complete a build, we
# can't depend on the environment even initialising properly. For example, if
# the type for a flag changes, any previously-saved value for the flag will
# be invalid.
if autogen_env['reset_flags']:
    autogen_env['reset_flags'] = False

    if os.path.isfile('options.cache'):
        processed_lines = []
        with open('options.cache') as f:
            for line in f:
                fields = [x.strip() for x in line.split('=', 1)]
                if not fields[0].lower().endswith('flags'):
                    processed_lines.append(line)

        with open('options.cache', 'w') as f:
            f.write('\n'.join(processed_lines))

# Explicitly limit the set of tools to try and find instead of using 'default'.
# This stops SCons from trying to find Fortran tools and the like.
tools_to_find = [
    'gnulink', 'gcc', 'g++', 'gas', 'ar', 'textfile', 'filesystem', 'tar',
    'cc', 'c++', 'link',
]

# Copy the host environment and install our options. If we use env.Platform()
# after this point, the Platform() call will override ENV and we don't want that
# or env['ENV']['PATH'] won't be the user's $PATH from the shell environment.
# That specifically breaks the build on OS X when using tar from macports (which
# is needed at least on OS X 10.5 as the OS X tar does not have --transform).
system_path = os.environ.get('PATH', '')
try:
    env = Environment(options=opts, platform='posix', ENV={'PATH': system_path},
                      tools=tools_to_find, TARFLAGS='-cz')
except SCons.Errors.EnvironmentError:
    tools_to_find.remove('textfile')
    env = Environment(options=opts, platform='posix', ENV={'PATH': system_path},
                      tools=tools_to_find, TARFLAGS='-cz')
Help(opts.GenerateHelpText(env))

def memoized_scons_subst(cache, old_scons_subst, string, env, mode=SCons.Subst.SUBST_RAW, target=None, source=None, gvars={}, lvars={}, conv=None):
    key_target = []
    key_source = []
    if target is not None:
        key_target = target
    if source is not None:
        key_source = source

    key = (string, env, mode, tuple(lvars.get('TARGETS', key_target)), tuple(lvars.get('SOURCES', key_source)))
    m = cache.get(key)
    if m is not None:
        return m

    m = old_scons_subst(string, env, mode, target, source, gvars, lvars, conv)

    cache[key] = m
    return m

# Swap out subst() functions with some that can actually cache.
import functools
scons_subst_cache = {}
scons_subst_list_cache = {}
SCons.Subst.scons_subst = functools.partial(memoized_scons_subst, scons_subst_cache, SCons.Subst.scons_subst)
SCons.Subst.scons_subst_list = functools.partial(memoized_scons_subst, scons_subst_list_cache, SCons.Subst.scons_subst_list)

# Perform timestamp checks first, then MD5 checks, to figure out if things change.
env.Decider('MD5-timestamp')

# Avoid any form of RCS scan (git etc)
env.SourceCode('.', None)

# Cache file checksums after 60 seconds
SetOption('max_drift', 60)

# Restrict suffixes we consider C++ to avoid searching for Fortran, .m, etc
env['CPPSUFFIXES'] = ['.c', '.C', '.cc', '.h', '.hpp', '.cpp', '.S']

# Bring across autogen variables to the main environment.
for k in autogen_opts.keys():
    env[k] = autogen_env.get(k)

# Look for things we care about for the build.
env['QEMU_IMG'] = env.Detect('qemu-img')
env['LOSETUP'] = env.Detect('losetup')
env['MKE2FS'] = env.Detect('mke2fs')
env['DEBUGFS'] = env.Detect('debugfs')
env['CCACHE'] = env.Detect('ccache')
env['DISTCC'] = env.Detect('distcc')
env['PYFLAKES'] = env.Detect('pyflakes')
env['MTOOLS_MMD'] = env.Detect('mmd')
env['MTOOLS_MCOPY'] = env.Detect('mcopy')
env['MTOOLS_MDEL'] = env.Detect('mdel')
env['MKISOFS'] = env.Detect(['mkisofs', 'genisoimage'])
env['SQLITE'] = env.Detect('sqlite3')
env['MKIMAGE'] = env.Detect('mkimage')
env['GIT'] = env.Detect('git')

# If we're on OSX, make sure we're using a sane tar.
if sys.platform == 'darwin':
    # Don't override an overridden TAR variable.
    if env['TAR'] == 'tar':
        env['TAR'] = env.Detect('gnutar')

# Look for things we absolutely must have.
required_tools = ['SQLITE', 'TAR']
if not all([env[x] is not None for x in required_tools]):
    raise SCons.Errors.UserError('Could not find all needed tools (need: %r)' % required_tools)

# Confirm whether we're making an ISO or not.
if env['MKISOFS'] is None:
    print 'No program to generate ISOs, not generating an ISO.'
    env['noiso'] = True

# Can we even build an image?
if not any([env[x] is not None for x in ['LOSETUP', 'MKE2FS', 'MTOOLS_MMD']]):
    msg = 'Cannot create a disk image by any means.'
    if env['distdir']:
        print msg
    else:
        raise SCons.Errors.UserError(msg)

# Determine whether we're cross-compiling or not.
if env.get('HOST_PLATFORM') is None:
    host = os.uname()
    env['ON_PEDIGREE'] = host[0] == 'Pedigree'
    env['HOST_PLATFORM'] = host[4]

# Enforce pre-commit hook.
if not os.path.exists('.git/hooks/pre-commit'):
    print "Enforcing pre-commit script."
    os.symlink('../../scripts/pre-commit.sh', '.git/hooks/pre-commit')

# Reset object file suffixes.
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = ''

# Reset file build suffixes (defaults are not always correct on each platform).
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = ''

# Determine build directories (these can be outside the source tree).
env['BUILDDIR'] = env.Dir(env['BUILDDIR']).abspath  # Normalise path.
env['PEDIGREE_BUILD_BASE'] = env['BUILDDIR']
env['PEDIGREE_BUILD_MODULES'] = os.path.join(env['BUILDDIR'], 'modules')
env['PEDIGREE_BUILD_KERNEL'] = os.path.join(env['BUILDDIR'], 'kernel')
env['PEDIGREE_BUILD_DRIVERS'] = os.path.join(env['BUILDDIR'], 'drivers')
env['PEDIGREE_BUILD_SUBSYS'] = os.path.join(env['BUILDDIR'], 'subsystems')
env['PEDIGREE_BUILD_APPS'] = os.path.join(env['BUILDDIR'], 'apps')

# Set up compilers and in particular the cross-compile environment.
if env['CROSS'] or env['ON_PEDIGREE']:
    if env['ON_PEDIGREE']:
        # TODO: parse 'gcc -v' to get COMPILER_TARGET
        env['COMPILER_TARGET'] = 'FIXME'
        crossPath = crossTuple = ''
    else:
        cross = os.path.split(env['CROSS'])
        crossPath = cross[0]
        crossTuple = cross[1].strip('-')

        env['COMPILER_TARGET'] = crossTuple
        if 'i686' in crossTuple:
            raise SCons.Errors.UserError('Please run "easy_build_x64.sh" to '
                'create a 64-bit toolchain: 32-bit builds of Pedigree are no '
                'longer supported.')

    tools = {}
    for tool in ['gcc', 'g++', 'as', 'ar', 'ranlib', 'strip', 'objcopy']:
        if crossTuple:
            tool_name = '%s-%s' % (crossTuple, tool)
        else:
            tool_name = tool
        tool_path = os.path.join(os.path.abspath(crossPath), tool_name)
        if os.path.isfile(tool_path):
            tools[tool] = tool_path
        else:
            raise SCons.Errors.UserError('Needed tool "%s" not found for '
                'target "%s".' % (tool, crossTuple))

    # Reset the compiler version as needed.
    # XXX: nasty hack, but older SCons doesn't allow a CC override.
    SCons.Tool.gcc.compilers = [tools['gcc']]
    env.Tool('gcc')

    env['XCOMPILER_PATH'] = crossPath

    prefix = ''
    if env['DISTCC'] is not None:
        prefix = '%s %s' % (env['DISTCC'], prefix)
    if env['CCACHE'] is not None:
        prefix = '%s %s' % (env['CCACHE'], prefix)
    prefix = prefix.strip()

    env['CC'] = '%s %s' % (prefix, tools['gcc'])
    env['CXX'] = '%s %s' % (prefix, tools['g++'])
    env['AS'] = tools['as']

    env['AR'] = tools['ar']
    env['RANLIB'] = tools['ranlib']
    env['LINK'] = tools['gcc']
    env['STRIP'] = tools['strip']
    env['OBJCOPY'] = tools['objcopy']

if env['ON_PEDIGREE'] or env['COMPILER_TARGET']:
    if env['ON_PEDIGREE']:
        host_arch = env['HOST_PLATFORM']
    else:
        host_arch = env['COMPILER_TARGET']

    extra_defines = []
    if re.match('i[3456]86', host_arch) is not None:
        flags_arch = 'x86'

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x86']
        env['ARCH_TARGET'] = 'X86'
    elif re.match('amd64|x86[_-]64', host_arch) is not None:
        flags_arch = 'x64'

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x64']
        env['ARCH_TARGET'] = 'X64'
    elif re.match('ppc|powerpc', host_arch) is not None:
        flags_arch = 'ppc'

        extra_defines += ['PPC']
        env['ARCH_TARGET'] = 'PPC'
    elif re.match('arm',host_arch) is not None:
        flags_arch = 'arm'

        # Handle input options
        mach = ''
        if env['arm_integrator']:
            extra_defines += ['ARM_INTEGRATOR']
            mach = 'integrator'
        elif env['arm_versatile']:
            extra_defines += ['ARM_VERSATILE']
            mach = 'versatile'
        elif env['arm_beagle']:
            extra_defines += ['ARM_BEAGLE']
            mach = 'beagle'

        ccflags = default_flags['arm']
        cflags = default_cflags['arm']
        cxxflags = default_cxxflags['arm']
        default_linkflags['arm'] = [x.replace('[mach]', mach) for x in
                                    default_linkflags['arm']]

        if env['arm_9']:
            extra_defines += ['ARM926E'] # TODO: too specific.
        elif env['armv7']:
            extra_defines += ['ARMV7']
            if env['arm_cortex_a8']:
                # TODO: actually need to use VFP, not FPA
                arm_flags = ['-mcpu=cortex-a8', '-mtune=cortex-a8', '-mfpu=vfp']
                ccflags += arm_flags

        default_flags['arm'] = ccflags
        default_cflags['arm'] = cflags
        default_cxxflags['arm'] = cxxflags

        if env['arm_bigendian']:
            extra_defines += ['BIG_ENDIAN']
        else:
            extra_defines += ['LITTLE_ENDIAN']

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['arm']
        env['ARCH_TARGET'] = 'ARM'

    flags = default_flags.get(flags_arch)
    if flags is not None:
        mapping = {
            'CCFLAGS': default_flags[flags_arch],
            'CFLAGS': default_cflags[flags_arch],
            'CXXFLAGS': default_cxxflags[flags_arch],
            'ASFLAGS': default_asflags[flags_arch],
            'LINKFLAGS': default_linkflags[flags_arch],
        }

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir[flags_arch]
        defines = default_defines[flags_arch] + extra_defines

        env.MergeFlags(mapping)

# Handle no valid target sensibly.
if not env['ARCH_TARGET'] and not env['ON_PEDIGREE']:
    raise SCons.Errors.UserError('Unsupported target - have you used '
        'scripts/checkBuildSystem.pl to build a cross-compiler?')

# Machine selection (this almost certainly shouldn't be here).
# TODO(miselin) figure out how to do this better
if env['ARCH_TARGET'] in ['X86', 'X64']:
    env['mach_pc'] = 1

# Override images directory if Pup is desired.
if env['pup']:
    env['PEDIGREE_IMAGES_DIR'] = '#images/local'

    imagesdir = env.Dir(env['PEDIGREE_IMAGES_DIR']).abspath
    if not os.path.isdir(imagesdir):
        os.makedirs(imagesdir)

# Handle extra debugging components.
if env['debugger']:
    # Build in debugging information when built with the debugger.
    # Use DWARF, as the stabs format is not very useful (32 bits of reloc)
    debug_flags = {'CCFLAGS': ['-g3', '-ggdb', '-gdwarf-2']}
    env.MergeFlags(debug_flags)

    # TODO(miselin): do the same for GNU AS targets

# NASM is used for X86 and X64 builds
if env['ARCH_TARGET'] in ('X86', 'X64'):
    env['AS'] = None
    if env['ON_PEDIGREE']:
        env['AS'] = env.Detect('nasm')
    else:
        env['AS'] = os.path.join(env['XCOMPILER_PATH'], 'nasm')
if env['AS'] is None:
    raise SCons.Errors.UserError('No assembler was found - make sure nasm/as are installed.')

# Add optional flags.
warning_flag = '-Wno-error'
if env['warnings']:
    warning_flag = '-Werror'
env.MergeFlags({'CFLAGS': warning_flag})

if env['memory_log']:
    defines.append('MEMORY_LOGGING_ENABLED')

if env['linux']:
    # TODO(miselin): do this better.
    defines = [x for x in defines if x not in ["X86_COMMON"]]

additionalDefines = ['ipv4_forwarding', 'serial_is_file', 'installer', 'debugger', 'cripple_hdd', 'enable_ctrlc',
                     'multiple_consoles', 'multiprocessor', 'smp', 'apic', 'acpi', 'debug_logging', 'superdebug', 'usb_verbose_debug',
                     'nogfx', 'mach_pc', 'memory_tracing', 'memory_log_inline', 'travis', 'linux']
for i in additionalDefines:
    if i not in env:
        continue

    if env[i]:
        defines += [i.upper()]

# TODO(miselin): figure out how to do dependent flags
if(env['multiprocessor'] or env['smp']):
    pass
    # defines = misc.safeAppend(defines, ['MULTIPROCESSOR'])
    # defines = misc.safeAppend(defines, ['APIC'])
    # defines = misc.safeAppend(defines, ['ACPI'])
    # defines = misc.safeAppend(defines, ['SMP'])

# Set the environment flags
env['CPPDEFINES'] = list(set(defines))

# Grab the date (rather than using the `date' program)
env['PEDIGREE_BUILDTIME'] = datetime.today().isoformat()

# Use the OS to find out information about the user and computer name
env['PEDIGREE_USER'] = getpass.getuser()
env['PEDIGREE_MACHINE'] = gethostname() # The name of the computer (not the type or OS)

# Grab the git revision of the repo
if env['GIT'] and env['genversion'] == '1':
    env['PEDIGREE_REVISION'] = commands.getoutput('%s rev-parse --verify HEAD --short' % env['GIT'])
else:
    env['PEDIGREE_REVISION'] = "(unknown)"

# Set the flags
env['PEDIGREE_FLAGS'] = ' '.join(env['CPPDEFINES'])

version_out = ['const char *g_pBuildTime = "$buildtime";',
               'const char *g_pBuildRevision = "$rev";',
               'const char *g_pBuildFlags = "$flags";',
               'const char *g_pBuildUser = "$user";',
               'const char *g_pBuildMachine = "$machine";',
               'const char *g_pBuildTarget = "$target";']

sub_dict = {"$buildtime"    : env['PEDIGREE_BUILDTIME'],
            "$rev"          : env['PEDIGREE_REVISION'],
            "$flags"        : env['PEDIGREE_FLAGS'],
            "$user"         : env['PEDIGREE_USER'],
            "$machine"      : env['PEDIGREE_MACHINE'],
            "$target"       : env['ARCH_TARGET']
           }

# Create the file
def create_version_cc(target, source, env):
    global version_out

    # We need to have a Version.cc, but we can disable the (costly) rebuild of
    # it every single time a compile is done - handy for developers.
    if env['genversion'] == '0' and os.path.exists(target[0].abspath):
        return

    # Make the non-SCons target a bit special.
    # People using Cygwin have enough to deal with without boring
    # status messages from build systems that don't support fancy
    # builders to do stuff quickly and easily.
    info = "Version.cc [rev: %s, with: %s@%s]" % (env['PEDIGREE_REVISION'], env['PEDIGREE_USER'], env['PEDIGREE_MACHINE'])
    if env['verbose']:
        print "      Creating %s" % (info,)
    else:
        print '      Creating \033[32m%s\033[0m' % (info,)

    def replacer(s):
        for keyname, value in sub_dict.iteritems():
            s = s.replace(keyname, value)
        return s
    
    version_out = map(replacer, version_out)
    
    f = open(target[0].path, 'w')
    f.write('\n'.join(version_out))
    f.close()
    
env.Command(os.path.join(env['PEDIGREE_BUILD_BASE'], 'Version.cc'), None, Action(create_version_cc, None))

# Override CXX if needed.
if env['iwyu']:
    # Make sure IWYU is fully freestanding and only refers to our headers.
    env['iwyu'] += ' -Xiwyu --no_default_mappings'
    env['iwyu'] += ' -Xiwyu --transitive_includes_only'

    env['CXXFLAGS'] = env['CFLAGS'] = []

    env['CC'] = env['iwyu']
    env['CXX'] = env['iwyu']

    # We don't want disk images when doing an IWYU run.
    env['nodiskimages'] = True

# Save the cache, all the options are configured
if env['cache']:
    opts.Save('options.cache', env)
    for k in autogen_opts.keys():
        autogen_env[k] = env[k]
    autogen_opts.Save('.autogen.cache', autogen_env)

# Make build messages much prettier.
misc.prettifyBuildMessages(env)

# Generate custom builders and add to environment.
misc.generate(env)

SConscript('SConscript', variant_dir=env['BUILDDIR'], exports=['env'], duplicate=0)

print
print "**** This build of Pedigree (at rev %s, for %s, by %s) started at %s ****" % (env['PEDIGREE_REVISION'], env['ARCH_TARGET'], env['PEDIGREE_USER'], datetime.today())
print

