####################################
# SCons build system for Pedigree
## Tyler Kennedy (AKA Linuxhq AKA TkTech)
## ToDO:
## ! Make the script suck less
## ! Add a check for -fno-stack-protector
## ! Make a flexible system for build defines
####################################
EnsureSConsVersion(0,98,0)

####################################
# Import various python libraries
## Do not use anything non-std
####################################
import os
import sys
import commands
import re
import string
import getpass
import SCons
from socket import gethostname
from datetime import *

# Grab all the default flags for each architecture
sys.path += ['./scripts']
from defaultFlags import *

####################################
# Default build flags (Also used to auto-generate help)
####################################
opts = Variables('options.cache')
#^-- Load saved settings (if they exist)
opts.AddVariables(
    ('CROSS','Base for cross-compilers, tool names will be appended automatically',''),
    ('CC','Sets the C compiler to use.'),
    ('CC_NOCACHE','Sets the non-ccached C compiler to use (defaults to CC).', ''),
    ('CXX','Sets the C++ compiler to use.'),
    ('AS','Sets the assembler to use.'),
    ('AR','Sets the archiver to use.'),
    ('LINK','Sets the linker to use.'),
    ('STRIP','Path to the `strip\' executable.'),
    ('OBJCOPY','Path to `objcopy\' executable.'),
    ('CFLAGS','Sets the C compiler flags.',''),
    ('CXXFLAGS','Sets the C++ compiler flags.',''),
    ('ASFLAGS','Sets the assembler flags.',''),
    ('LINKFLAGS','Sets the linker flags',''),
    ('BUILDDIR','Directory to place build files in.','build'),
    ('LIBGCC','The folder containing libgcc.a.',''),
    ('LOCALISEPREFIX', 'Prefix to add to all compiler invocations whose output is parsed. Used to ensure locales are right, this must be a locale which outputs US English.', 'LANG=C'),
    ('COMPILER_TARGET', 'Compiler target (eg, i686-pedigree). Autodetected.', ''),
    ('COMPILER_VERSION', 'Compiler version (eg, 4.5.1). Autodetected.', ''),

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
    ('iwyu', 'If set, use the given as a the C++ compiler for include-what-you-use. Use -i with scons if you use IWYU.', ''),
    
    BoolVariable('nocache', 'Do not create an options.cache file (NOT recommended).', 0),
    BoolVariable('genversion', 'Whether or not to regenerate Version.cc if it already exists.', 1),
    
    ('distdir', 'Directory to install a Pedigree directory structure to, instead of a disk image. Empty will use disk images.', ''),
    BoolVariable('havelosetup', 'Whether or not `losetup` is available.', 0),
    BoolVariable('forcemtools', 'Force use of mtools (and the FAT filesystem) even if losetup is available.', 1),
    BoolVariable('haveqemuimg', 'Whether or not `qemu-img` is available (for VDI/VMDK creation).', 0),
    BoolVariable('createvdi', 'Convert the created hard disk image to a VDI file for VirtualBox after it is created.', 0),
    BoolVariable('createvmdk', 'Convert the created hard disk image to a VMDK file for VMware after it is created.', 0),
    BoolVariable('nodiskimages', 'Whether or not to avoid building disk images for distribution.', 0),
    BoolVariable('noiso', 'Whether or not to avoid building an ISO.', 0),
    ('isoprog', 'Program to use to generate ISO images. The default of `mkisofs\' should be fine for most.', 'mkisofs'),
    
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

    # ARM options
    BoolVariable('arm_integrator', 'Target the Integrator/CP development board', 0),
    BoolVariable('arm_versatile', 'Target the Versatile/PB development board', 0),
    BoolVariable('arm_beagle', 'Target the BeagleBoard', 0),

    BoolVariable('arm_9', 'Target the ARM9 processor family (currently only ARM926E)', 0),
    BoolVariable('armv7', 'Target the ARMv7 architecture family', 0),

    BoolVariable('arm_cortex_a8', 'Tune and optimise for the ARM Cortex A8 (use in conjunction with the `armv7\' option.', 0),

    BoolVariable('arm_bigendian', 'Is this ARM target big-endian?', 0),
    
    ('uimage_target', 'Where to copy the generated uImage.bin file to.', '~'),

    ####################################
    # These options are NOT TO BE USED on the command line!
    # They're here because they need to be saved through runs.
    ####################################
    ('CPPDEFINES', 'Final set of preprocessor definitions (DO NOT USE)', ''),
    ('ARCH_TARGET', 'Automatically generated architecture name (DO NOT USE)', ''),
)

# Copy the host environment and install our options. If we use env.Platform()
# after this point, the Platform() call will override ENV and we don't want that
# or env['ENV']['PATH'] won't be the user's $PATH from the shell environment.
# That specifically breaks the build on OS X when using tar from macports (which
# is needed at least on OS X 10.5 as the OS X tar does not have --transform).
try:
    env = Environment(options=opts, ENV=os.environ, platform='posix',
                      tools = ['default', 'textfile'], TARFLAGS='-cz')
except SCons.Errors.EnvironmentError:
    env = Environment(options=opts, ENV=os.environ, platform='posix',
                      tools = ['default'], TARFLAGS='-cz')
Help(opts.GenerateHelpText(env))

host = os.uname()

env['ON_PEDIGREE'] = host[0] == 'Pedigree'
env['HOST_PLATFORM'] = host[4]

# Don't use MD5s to determine if files have changed, just check the timestamp
env.Decider('timestamp-newer')

# Avoid any form of RCS scan (git etc)
env.SourceCode(".", None)

# Cache file checksums after 60 seconds
SetOption('max_drift', 60)

# Reset the suffixes
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = ''

# Grab the locale prefix
localisePrefix = env['LOCALISEPREFIX']
if(len(localisePrefix) > 0):
    localisePrefix += ' '

# Pedigree binary locations
env['PEDIGREE_BUILD_BASE'] = env['BUILDDIR']
env['PEDIGREE_BUILD_MODULES'] = env['BUILDDIR'] + '/modules'
env['PEDIGREE_BUILD_KERNEL'] = env['BUILDDIR'] + '/kernel'
env['PEDIGREE_BUILD_DRIVERS'] = env['BUILDDIR'] + '/drivers'
env['PEDIGREE_BUILD_SUBSYS'] = env['BUILDDIR'] + '/subsystems'
env['PEDIGREE_BUILD_APPS'] = env['BUILDDIR'] + '/apps'

def safeAppend(a, b):
    a = a.split()
    b = b.split()
    for item in b:
        if not item in a:
            a.append(item)
    return ' '.join(a)

if env['CC_NOCACHE'] == '':
    env['CC_NOCACHE'] = env['CC']

# Set the compilers if CROSS is not an empty string
if env['CROSS'] != '' or env['ON_PEDIGREE']:
    if not env['ON_PEDIGREE']:
        cross = os.path.split(env['CROSS'])
        crossPath = cross[0]
        crossTuple = cross[1]

        env['COMPILER_TARGET'] = crossTuple.strip('-')
        if 'i686' in crossTuple:
            print "Please run 'easy_build_x64.sh' to create a 64-bit toolchain."
            print "32-bit builds of Pedigree are no longer supported."
            exit(1)
    else:
        # TODO: parse 'gcc -v' to get COMPILER_TARGET
        env['COMPILER_TARGET'] = 'FIXME'
        crossPath = crossTuple = ''

    if crossPath:
        env['ENV']['PATH'] =  os.path.abspath(crossPath) + ':' + env['ENV']['PATH']

    prefix = ''
    if env['distcc']:
        prefix = 'distcc '
    if env['ccache']:
        prefix = 'ccache ' + prefix
    env['CC'] = prefix + crossTuple + 'gcc'
    env['CC_NOCACHE'] = crossTuple + 'gcc'
    env['CXX'] = prefix + crossTuple + 'g++'
    
    # AS will be setup soon
    env['AS'] = ''

    # AR and LD never have the prefix added. They must run on the host.
    env['AR'] = crossTuple + 'ar'
    env['RANLIB'] = crossTuple + 'ranlib'
    env['LD'] = crossTuple + 'gcc'
    env['STRIP'] = crossTuple + 'strip'
    env['OBJCOPY'] = crossTuple + 'objcopy'
    env['LINK'] = env['LD']

    p = commands.getoutput('%s -v' % (env['CC'],))
    for line in p.splitlines():
        if line.startswith('gcc version'):
            # TODO: fix this, this is horrible!
            env['COMPILER_VERSION'] = line.split(' ')[2]

env['LD'] = env['LINK']

if(len(env['CC']) > 0 and env['LIBGCC'] == ''):
    a = os.popen(env['CC'] + ' --print-libgcc-file-name')
    env['LIBGCC'] = os.path.dirname(a.read())
    a.close()

if env['haveqemuimg']:
    p = commands.getoutput("which qemu-img")
    if not os.path.exists(p):
        env['haveqemuimg'] = False

# Verify the ISO program
if not env['noiso']:
    p = commands.getoutput("which " + env['isoprog'])
    if not os.path.exists(p):
        print "No program to generate ISOs, not generating an ISO."
        env['noiso'] = True

tmp = re.match('(.*?)\-.*', os.path.basename(env['CROSS']), re.S)
if(env['ON_PEDIGREE'] or tmp != None):
    if env['ON_PEDIGREE']:
        host_arch = env['HOST_PLATFORM']
    else:
        host_arch = tmp.group(1)

    if re.match('i[3456]86',host_arch) != None:
        defines = default_defines['x86']
        env['CFLAGS'] = safeAppend(env['CFLAGS'], default_cflags['x86'])
        env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], default_cxxflags['x86'])
        env['ASFLAGS'] = safeAppend(env['ASFLAGS'], default_asflags['x86'])
        env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], default_linkflags['x86'])
        
        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x86']
        env['ARCH_TARGET'] = 'X86'
    elif re.match('amd64|x86[_-]64',host_arch) != None:
        defines = default_defines['x64']
        env['CFLAGS'] = safeAppend(env['CFLAGS'], default_cflags['x64'])
        env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], default_cxxflags['x64'])
        env['ASFLAGS'] = safeAppend(env['ASFLAGS'], default_asflags['x64'])
        env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], default_linkflags['x64'])
        
        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x64']
        env['ARCH_TARGET'] = 'X64'
    elif re.match('ppc|powerpc',host_arch) != None:
        defines += ['PPC']

        env['ARCH_TARGET'] = 'PPC'
    elif re.match('arm',host_arch) != None:
        defines = default_defines['arm']


        # Handle input options
        mach = ''
        if env['arm_integrator']:
            defines += ['ARM_INTEGRATOR']
            mach = 'integrator'
        elif env['arm_versatile']:
            defines += ['ARM_VERSATILE']
            mach = 'versatile'
        elif env['arm_beagle']:
            defines += ['ARM_BEAGLE']
            mach = 'beagle'

        cflags = default_cflags['arm']
        cxxflags = default_cxxflags['arm']
        asflags = default_asflags['arm']
        linkflags = default_linkflags['arm'].replace('[mach]', mach)

        if env['arm_9']:
            defines += ['ARM926E'] # TODO: too specific.
        elif env['armv7']:
            defines += ['ARMV7']
            if env['arm_cortex_a8']:
                # TODO: actually need to use VFP, not FPA
                cflags = safeAppend(cflags, ' -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=fpa ')
                cxxflags = safeAppend(cxxflags, ' -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=fpa ')

        env['CFLAGS'] = safeAppend(env['CFLAGS'], cflags)
        env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], cxxflags)
        env['ASFLAGS'] = safeAppend(env['ASFLAGS'], asflags)
        env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], linkflags)
        
        if env['arm_bigendian']:
            defines += ['BIG_ENDIAN']
        else:
            defines += ['LITTLE_ENDIAN']

        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['arm']
        env['ARCH_TARGET'] = 'ARM'

if(tmp == None or env['ARCH_TARGET'] == ''):
    if not env['ON_PEDIGREE']:
        print "Unsupported target - have you used scripts/checkBuildSystem.pl to build a cross-compiler?"
        Exit(1)

if(env['pup']):
    env['PEDIGREE_IMAGES_DIR'] = '#images/local/'

if env['debugger']:
    # Build in debugging information when built with the debugger.
    # Use DWARF, as the stabs format is not very useful (32 bits of reloc)
    debug_flags = ' -g3 -ggdb -gdwarf-2 '
    env['CFLAGS'] = safeAppend(env['CFLAGS'], debug_flags)
    env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], debug_flags)

    # TODO: for targets using `as` instead of `nasm`, this can in fact be DWARF!
    debug_flags = ''
    env['ASFLAGS'] = safeAppend(env['ASFLAGS'], debug_flags)
    
# Configure the assembler
if(env['AS'] == ''):
    # NASM is used for X86 and X64 builds
    if env['ARCH_TARGET'] == 'X86' or env['ARCH_TARGET'] == 'X64':
        if env['ON_PEDIGREE']:
            env['AS'] = 'nasm'
        else:
            crossPath = os.path.dirname(env['CROSS'])
            env['AS'] = crossPath + "/nasm"
    else:
        if env['ON_PEDIGREE']:
            env['AS'] = 'as'
        else:
           if(env['CROSS'] == ''):
               print "Error: Please set AS on the command line."
               Exit(1)
           env['AS'] = env['CROSS'] + "as"

# Detect losetup presence
if not env['forcemtools']:
    tmp = commands.getoutput("which losetup")
    if(len(tmp) and not "no losetup" in tmp):
        env['havelosetup'] = 1
    else:
        env['havelosetup'] = 0
else:
    env['havelosetup'] = 0

# Extra build flags
if not env['warnings'] and not '-Werror' in env['CXXFLAGS']:
    env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], ' -Werror')
    env['CFLAGS'] = safeAppend(env['CFLAGS'], ' -Werror')
elif '-Werror' in env['CXXFLAGS']:
    env['CXXFLAGS'] = env['CXXFLAGS'].replace('-Werror', '')
    env['CFLAGS'] = env['CFLAGS'].replace('-Werror', '')

if env['verbose_link'] and not '--verbose' in env['LINKFLAGS']:
    env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], ' --verbose')
elif '--verbose' in env['LINKFLAGS']:
    env['LINKFLAGS'] = env['LINKFLAGS'].replace('--verbose', '')

if env['memory_tracing']:
    defines += ['MEMORY_TRACING']
elif env['memory_log']:
    defines += ['MEMORY_LOGGING_ENABLED']
elif env['memory_log_inline']:
    defines += ['MEMORY_LOG_INLINE']
    
additionalDefines = ['ipv4_forwarding', 'serial_is_file', 'installer', 'debugger', 'cripple_hdd', 'enable_ctrlc',
                     'multiple_consoles', 'multiprocessor', 'smp', 'apic', 'acpi', 'debug_logging', 'superdebug', 'usb_verbose_debug',
                     'nogfx']
for i in additionalDefines:
    if(env[i] and not i in defines):
        defines += [i.upper()]
if(env['multiprocessor'] or env['smp']):
    pass
    # defines = safeAppend(defines, ['MULTIPROCESSOR'])
    # defines = safeAppend(defines, ['APIC'])
    # defines = safeAppend(defines, ['ACPI'])
    # defines = safeAppend(defines, ['SMP'])

# Set the environment flags
env['CPPDEFINES'] = defines

####################################
# Fluff up our build messages
####################################
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

####################################
# Generate Version.cc
# Exports:
## PEDIGREE_BUILDTIME
## PEDIGREE_REVISION
## PEDIGREE_FLAGS
## PEDIGREE_USER
## PEDIGREE_MACHINE
####################################

# Grab the date (rather than using the `date' program)
env['PEDIGREE_BUILDTIME'] = datetime.today().isoformat()

# Use the OS to find out information about the user and computer name
env['PEDIGREE_USER'] = getpass.getuser()
env['PEDIGREE_MACHINE'] = gethostname() # The name of the computer (not the type or OS)

# Grab the git revision of the repo
gitpath = commands.getoutput("which git")
if os.path.exists(gitpath) and env['genversion'] == '1':
    env['PEDIGREE_REVISION'] = commands.getoutput(gitpath + ' rev-parse --verify HEAD --short')
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
    
env.Command('#' + env['BUILDDIR'] + '/Version.cc', None, Action(create_version_cc, None))

# Override CXX if needed.
if env['iwyu']:
    oldflags = env['CXXFLAGS']

    # Make sure IWYU is fully freestanding and only refers to our headers.
    env['CXXFLAGS'] = '-Xiwyu --no_default_mappings -Xiwyu --transitive_includes_only '

    env['CXXFLAGS'] += re.sub('\-march\=.* \-mtune\=.*', '', oldflags)
    env['CXXFLAGS'] += ' -target i686-unknown-elf '
    env['CXX'] = env['iwyu']

# Save the cache, all the options are configured
if(not env['nocache']):
    opts.Save('options.cache', env)

####################################
# Progress through all our sub-directories
####################################
SConscript('SConscript', variant_dir = env['BUILDDIR'], exports = ['env'], duplicate = 0)

print
print "**** This build of Pedigree (at rev %s, for %s, by %s) started at %s ****" % (env['PEDIGREE_REVISION'], env['ARCH_TARGET'], env['PEDIGREE_USER'], datetime.today())
print
