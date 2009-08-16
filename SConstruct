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
    'DEBUGGER',                 # Enable the debugger
    'DEBUGGER_QWERTY',          # Enable the QWERTY keymap
    #'SMBIOS',                  # Enable SMBIOS
    'SERIAL_IS_FILE',           # Don't treat the serial port like a VT100 terminal
    'ECHO_CONSOLE_TO_SERIAL',   # Puts the kernel's log over the serial port, (qemu -serial file:serial.txt or /dev/pts/0)
    'KERNEL_NEEDS_ADDRESS_SPACE_SWITCH',
    'ADDITIONAL_CHECKS',
    'BITS_32',
    'KERNEL_STANDALONE',
    'VERBOSE_LINKER',           # Increases the verbosity of messages from the Elf and KernelElf classes
    'TRACK_LOCKS',
]

# Default CFLAGS
# 
default_cflags = '-std=gnu99 -march=i486 -fno-builtin -nostdinc -nostdlib -ffreestanding -m32 -g0 -O3 '

# Default CXXFLAGS
# 
default_cxxflags = '-std=gnu++98 -march=i486 -fno-builtin -nostdinc -nostdlib -ffreestanding -fno-rtti -fno-exceptions -m32 -g0 -O3 '

# Entry level warning flags
default_cflags += '-Wno-long-long '
default_cxxflags += '-Weffc++ -Wold-style-cast '

# Extra warnings (common to C/C++)
extra_warnings = '-Wall -Wextra -Wpointer-arith -Wcast-align -Wwrite-strings -Wno-long-long -Wno-variadic-macros '
default_cflags += '-Wnested-externs '
default_cflags += extra_warnings
default_cxxflags += extra_warnings

# Turn off some stuff that kills the build
default_cflags += '-Wno-unused -Wno-unused-variable -Wno-conversion -Wno-format -Wno-empty-body'
default_cxxflags += '-Wno-unused -Wno-unused-variable -Wno-conversion -Wno-format -Wno-empty-body'

# Default linker flags
default_linkflags = '-T src/system/kernel/core/processor/x86/kernel.ld -nostdlib -nostdinc -nostartfiles'

####################################
# Default build flags (Also used to auto-generate help)
####################################
opts = Variables('options.cache')
#^-- Load saved settings (if they exist)
opts.AddVariables(
    ('CROSS','Base for cross-compilers, tool names will be appended automatically',''),
    ('CC','Sets the C compiler to use.'),
    ('CXX','Sets the C++ compiler to use.'),
    ('AS','Sets the assembler to use.'),
    ('AR','Sets the archiver to use.'),
    ('LINK','Sets the linker to use.'),
    ('CFLAGS','Sets the C compiler flags.',default_cflags),
    ('CXXFLAGS','Sets the C++ compiler flags.',default_cxxflags),
    ('ASFLAGS','Sets the assembler flags.','-f elf'),
    ('LINKFLAGS','Sets the linker flags',default_linkflags),
    ('BUILDDIR','Directory to place build files in.','build'),
    ('LIBGCC','The folder containing libgcc.a.',''),
    ('LOCALISEPREFIX', 'Prefix to add to all compiler invocations whose output is parsed. Used to ensure locales are right, this must be a locale which outputs US English.', 'LANG=C'),
    BoolVariable('verbose','Display verbose build output.',0),
    BoolVariable('nocolour','Don\'t use colours in build output.',0),
    BoolVariable('verbose_link','Display verbose linker output.',0),
    BoolVariable('warnings', 'If nonzero, compilation without -Werror', 1),
	BoolVariable('installer', 'Build the installer', 0),
    BoolVariable('genflags', 'Whether or not to generate flags and things dynamically.', 1),
    
    ####################################
    # These options are NOT TO BE USED on the command line!
    # They're here because they need to be saved through runs.
    ####################################
    ('CPPDEFINES', 'Final set of preprocessor definitions (DO NOT USE)', ''),
    ('ARCH_TARGET', 'Automatically generated architecture name (DO NOT USE)', ''),
)

env = Environment(options = opts, ENV = os.environ)
#^-- Create a new environment object passing the options
Help(opts.GenerateHelpText(env))
#^-- Create the scons help text from the options we specified earlier

# POSIX platform :)
env.Platform('posix')

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

# If we need to generate flags, do so
if env['genflags']:
    
    # Set the compilers if CROSS is not an empty string
    if env['CROSS'] != '':
        crossBase = env['CROSS']
        env['CC'] = crossBase + 'gcc'
        env['CXX'] = crossBase + 'g++'
        env['LD'] = crossBase + 'gcc'
        env['AS'] = crossBase + 'as'
        env['AR'] = crossBase + 'ar'
        env['LINK'] = env['LD']
    env['LD'] = env['LINK']

    ####################################
    # Compiler/Target specific settings
    ####################################
    # Check to see if the compiler supports --fno-stack-protector
    out = commands.getoutput('echo -e "int main(void) {return 0;}" | ' + localisePrefix + env['CC'] + ' -x c -fno-stack-protector -c -o /dev/null -')
    if not 'unrecognized option' in out:
        env['CXXFLAGS'] += ' -fno-stack-protector'
        env['CFLAGS'] += ' -fno-stack-protector'

    out = commands.getoutput(localisePrefix + env['CXX'] + ' -v')
    #^-- The old script used --dumpmachine, which isn't always present
    tmp = re.match('.*?Target: ([^\n]+)',out,re.S)

    if tmp != None:
        env['ARCH_TARGET'] = tmp.group(1)

        if re.match('i[3456]86',tmp.group(1)) != None:
            defines +=  ['X86','X86_COMMON','LITTLE_ENDIAN']
            #^-- Should provide overloads for these...like machine=ARM_VOLITILE
            
            env['ARCH_TARGET'] = 'X86'
        elif re.match('amd64|x86[_-]64',tmp.group(1)) != None:
            defines += ['X64']
            
            env['ARCH_TARGET'] = 'X64'
        elif re.match('ppc|powerpc',tmp.group(1)) != None:
            defines += ['PPC']
            
            env['ARCH_TARGET'] = 'PPC'
        elif re.match('arm',tmp.group(1)) != None:
            defines += ['ARM']
            
            env['ARCH_TARGET'] = 'ARM'

    # Default to x86 if something went wrong
    else:
        env['ARCH_TARGET'] = 'X86'
            
    # LIBGCC path
    if env['LIBGCC'] == '':
        # Because Windows is *gay* at running commands.getouput() properly...
        stdout = os.popen(localisePrefix + env['CXX'] + ' --print-libgcc-file-name')
        libgccPath = stdout.read().strip()
        stdout.close()
        if not os.path.exists(libgccPath):
            print "Error: libgcc path could not be determined. Use the LIBGCC option."
            Exit(1)
        
        # Not out of the woods yet, Windows-specific hackery!
        # Only required if you use a MinGW-based cross-compiler (they're faster <3)
        if ':' in libgccPath:
            # Probably a Windows path... Colons really shouldn't be anywhere else
            drive = libgccPath[0:1]
            libgccPath = libgccPath[len('a:'):]
            
            # Assume Cygwin. Someone's going to kill me over this.
            libgccPath = "/cygdrive/" + drive + libgccPath
        
        env['LIBGCC'] = os.path.dirname(libgccPath)

    # NASM is used for X86 and X64 builds
    if env['ARCH_TARGET'] == 'X86' or env['ARCH_TARGET'] == 'X64':
        crossPath = os.path.dirname(env['CC'])
        env['AS'] = crossPath + '/nasm'

    ####################################
    # Some quirks
    ####################################
    defines += ['__UD_STANDALONE__']
    #^-- Required no matter what.

    if not env['warnings']:
        env['CXXFLAGS'] += ' -Werror'

    if env['verbose_link']:
        env['LINKFLAGS'] += ' --verbose'

    if env['installer']:
        defines += ['INSTALLER']

    ####################################
    # Setup our build options
    ####################################
    env['CPPDEFINES'] = []
    env['CPPDEFINES'] = [i for i in defines]
    #^-- Stupid, I know, but again I plan on adding some preprocessing (AKA auto-options for architecutres)
    
    # Don't regenerate until we really have to
    env['genflags'] = False

    # Save the cache, all the options are configured
    opts.Save('options.cache',env)
    #^-- Save the cache file over the old one

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
