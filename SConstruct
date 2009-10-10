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
from socket import gethostname
from datetime import *
####################################
# Add an option here to have it defined
####################################
defines = [
    'THREADS',                  # Enable threading
    'DEBUGGER_QWERTY',          # Enable the QWERTY keymap
    #'SMBIOS',                  # Enable SMBIOS
    'SERIAL_IS_FILE',           # Don't treat the serial port like a VT100 terminal
    #'DONT_LOG_TO_SERIAL',      # Do not put the kernel's log over the serial port, (qemu -serial file:serial.txt or /dev/pts/0 or stdio on linux)
                                # TODO: Should be a proper option... I'll do that soon. -Matt
    'ADDITIONAL_CHECKS',
    'KERNEL_STANDALONE',
    'VERBOSE_LINKER',           # Increases the verbosity of messages from the Elf and KernelElf classes
]
    
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
    ('CFLAGS','Sets the C compiler flags.',''),
    ('CXXFLAGS','Sets the C++ compiler flags.',''),
    ('ASFLAGS','Sets the assembler flags.',''),
    ('LINKFLAGS','Sets the linker flags',''),
    ('BUILDDIR','Directory to place build files in.','build'),
    ('LIBGCC','The folder containing libgcc.a.',''),
    ('LOCALISEPREFIX', 'Prefix to add to all compiler invocations whose output is parsed. Used to ensure locales are right, this must be a locale which outputs US English.', 'LANG=C'),

    BoolVariable('cripple_hdd','Disable writing to hard disks at runtime.',1),
    BoolVariable('debugger','Whether or not to enable the kernel debugger.',1),

    BoolVariable('verbose','Display verbose build output.',0),
    BoolVariable('nocolour','Don\'t use colours in build output.',0),
    BoolVariable('verbose_link','Display verbose linker output.',0),
    BoolVariable('warnings', 'If nonzero, compilation without -Werror', 1),
    BoolVariable('installer', 'Build the installer', 0),
    BoolVariable('genflags', 'Whether or not to generate flags and things dynamically.', 1),
    BoolVariable('ccache', 'Prepend ccache to cross-compiler paths (for use with CROSS)', 0),
    
    BoolVariable('nocache', 'Do not create an options.cache file (NOT recommended).', 0),
    BoolVariable('genversion', 'Whether or not to regenerate Version.cc if it already exists.', 1),
    
    BoolVariable('havelosetup', 'Whether or not `losetup` is available.', 0),
    
    BoolVariable('pacman', 'If 1, you are managing your images/local directory with pacman and want that instead of the images/<arch> directory.', 0),

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
env = Environment(options=opts, ENV=os.environ, platform='posix')
Help(opts.GenerateHelpText(env))

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
    if not b in a:
        a += b
    return a

if env['CC_NOCACHE'] == '':
    env['CC_NOCACHE'] = env['CC']

# Set the compilers if CROSS is not an empty string
if env['CROSS'] != '':
    crossBase = env['CROSS']
    prefix = ''
    if env['ccache']:
        prefix = 'ccache '
    env['CC'] = prefix + crossBase + 'gcc'
    env['CC_NOCACHE'] = crossBase + 'gcc'
    env['CXX'] = prefix + crossBase + 'g++'
    
    # AS will be setup soon
    env['AS'] = ''

    # AR and LD never have the prefix added. They must run on the host.
    env['AR'] = crossBase + 'ar'
    env['LD'] = crossBase + 'gcc'
    env['LINK'] = env['LD']
env['LD'] = env['LINK']

tmp = re.match('(.*?)\-.*', os.path.basename(env['CROSS']), re.S)
if(tmp != None):
    if re.match('i[3456]86',tmp.group(1)) != None:
        defines = safeAppend(defines, default_defines['x86'])
        env['CFLAGS'] = safeAppend(env['CFLAGS'], default_cflags['x86'])
        env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], default_cxxflags['x86'])
        env['ASFLAGS'] = safeAppend(env['ASFLAGS'], default_asflags['x86'])
        env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], default_linkflags['x86'])
        
        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x86']
        env['ARCH_TARGET'] = 'X86'
    elif re.match('amd64|x86[_-]64',tmp.group(1)) != None:
        defines = safeAppend(defines, default_defines['x64'])
        env['CFLAGS'] = safeAppend(env['CFLAGS'], default_cflags['x64'])
        env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], default_cxxflags['x64'])
        env['ASFLAGS'] = safeAppend(env['ASFLAGS'], default_asflags['x64'])
        env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], default_linkflags['x64'])
        
        env['PEDIGREE_IMAGES_DIR'] = default_imgdir['x64']
        env['ARCH_TARGET'] = 'X64'
    elif re.match('ppc|powerpc',tmp.group(1)) != None:
        defines += ['PPC']

        env['ARCH_TARGET'] = 'PPC'
    elif re.match('arm',tmp.group(1)) != None:
        defines += ['ARM']

        env['ARCH_TARGET'] = 'ARM'
if(tmp == None or env['ARCH_TARGET'] == ''):
    print "Unsupported target - have you used scripts/checkBuildSystem.pl to build a cross-compiler?"
    Exit(1)

if(env['pacman']):
    env['PEDIGREE_IMAGES_DIR'] = '#images/local/'
    
# Configure the assembler
if(env['AS'] == ''):
    # NASM is used for X86 and X64 builds
    if env['ARCH_TARGET'] == 'X86' or env['ARCH_TARGET'] == 'X64':
        crossPath = os.path.dirname(env['CC'])
        env['AS'] = crossPath + "/nasm"
    else:
        if(env['CROSS'] == ''):
            print "Error: Please set AS on the command line."
            Exit(1)
        env['AS'] = env['CROSS'] + "as"

# Detect losetup presence
tmp = commands.getoutput("which losetup")
if(len(tmp) and not "no losetup" in tmp):
    env['havelosetup'] = 1
else:
    env['havelosetup'] = 0

# Extra build flags
if not env['warnings'] and not '-Werror' in env['CXXFLAGS']:
    env['CXXFLAGS'] = safeAppend(env['CXXFLAGS'], ' -Werror')
    env['CFLAGS'] = safeAppend(env['CFLAGS'], ' -Werror')
else:
    env['CXXFLAGS'] = env['CXXFLAGS'].replace('-Werror', '')
    env['CFLAGS'] = env['CFLAGS'].replace('-Werror', '')

if env['verbose_link'] and not '--verbose' in env['LINKFLAGS']:
    env['LINKFLAGS'] = safeAppend(env['LINKFLAGS'], ' --verbose')
else:
    env['LINKFLAGS'] = env['LINKFLAGS'].replace('--verbose', '')
    
additionalDefines = ['installer', 'debugger', 'cripple_hdd']
for i in additionalDefines:
    if(env[i] and not i in defines):
        defines += [i.upper()]

# Set the environment flags
env['CPPDEFINES'] = defines

# Save the cache, all the options are configured
if(not env['nocache']):
    opts.Save('options.cache', env)

####################################
# Fluff up our build messages
####################################
if not env['verbose']:
    if env['nocolour'] or os.environ['TERM'] == 'dumb' :
        env['CCCOMSTR']   =    '     Compiling $TARGET'
        env['CXXCOMSTR']  =    '     Compiling $TARGET'
        env['ASCOMSTR']   =    '    Assembling $TARGET'
        env['LINKCOMSTR'] =    '       Linking $TARGET'
        env['ARCOMSTR']   =    '     Archiving $TARGET'
        env['RANLIBCOMSTR'] =  '      Indexing $TARGET'
        env['NMCOMSTR']   =    '  Creating map $TARGET'
        env['DOCCOMSTR']  =    '   Documenting $TARGET'
    else:
        env['CCCOMSTR']   =    '     Compiling \033[32m$TARGET\033[0m'
        env['CXXCOMSTR']  =    '     Compiling \033[32m$TARGET\033[0m'
        env['ASCOMSTR']   =    '    Assembling \033[32m$TARGET\033[0m'
        env['LINKCOMSTR'] =    '       Linking \033[32m$TARGET\033[0m'
        env['ARCOMSTR']   =    '     Archiving \033[32m$TARGET\033[0m'
        env['RANLIBCOMSTR'] =  '      Indexing \033[32m$TARGET\033[0m'
        env['NMCOMSTR']   =    '  Creating map \033[32m$TARGET\033[0m'
        env['DOCCOMSTR']  =    '   Documenting \033[32m$TARGET\033[0m'

####################################
# Generate Version.cc
# Exports:
## PEDIGREE_BUILDTIME
## PEDIGREE_REVISION
## PEDIGREE_FLAGS
## PEDIGREE_USER
## PEDIGREE_MACHINE
####################################
if(env['genversion'] and not os.path.exists('src/system/kernel/Version.cc')):
    # Grab the date (rather than using the `date' program)
    env['PEDIGREE_BUILDTIME'] = datetime.today().isoformat()

    # Use the OS to find out information about the user and computer name
    env['PEDIGREE_USER'] = getpass.getuser()
    env['PEDIGREE_MACHINE'] = gethostname() # The name of the computer (not the type or OS)

    # Grab the git revision of the repo
    gitpath = commands.getoutput("which git")
    if os.path.exists(gitpath):
        env['PEDIGREE_REVISION'] = commands.getoutput(gitpath + ' rev-parse --verify HEAD --short')
    else:
        env['PEDIGREE_REVISION'] = "(unknown, git not found)"

    # Set the flags
    env['PEDIGREE_FLAGS'] = ' '.join(env['CPPDEFINES'])

    # Build the string of data to write
    version_out = ''
    version_out += 'const char *g_pBuildTime = "'     + env['PEDIGREE_BUILDTIME'] + '";\n'
    version_out += 'const char *g_pBuildRevision = "' + env['PEDIGREE_REVISION']  + '";\n'
    version_out += 'const char *g_pBuildFlags = "'    + env['PEDIGREE_FLAGS']     + '";\n'
    version_out += 'const char *g_pBuildUser = "'     + env['PEDIGREE_USER']      + '";\n'
    version_out += 'const char *g_pBuildMachine = "'  + env['PEDIGREE_MACHINE']   + '";\n'
    version_out += 'const char *g_pBuildTarget = "'   + env['ARCH_TARGET']        + '";\n'

    # Write the file to disk (We *assume* src/system/kernel/)
    file = open('src/system/kernel/Version.cc','w')
    file.write(version_out)
    file.close()

####################################
# Progress through all our sub-directories
####################################
SConscript('SConscript', exports = ['env'], build_dir = env['BUILDDIR'], duplicate = 0)
