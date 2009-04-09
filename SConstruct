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
import commands
import re
import string
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
]

####################################
# Default build flags (Also used to auto-generate help)
####################################
opts = Variables('options.cache')
#^-- Load saved settings (if they exist)
opts.AddVariables(
    ('CC','Sets the C compiler to use.'),
    ('CXX','Sets the C++ compiler to use.'),
    ('AS','Sets the assembler to use.'),
    ('LINK','Sets the linker to use.'),
    ('CFLAGS','Sets the C compiler flags.','-march=i486 -fno-builtin -nostdlib -m32 -g0 -O3'),
    ('CXXFLAGS','Sets the C++ compiler flags.','-march=i486 -fno-builtin -nostdlib -m32 -g0 -O3 -Weffc++ -Wold-style-cast -Wno-long-long -fno-rtti -fno-exceptions'),
    ('ASFLAGS','Sets the assembler flags.','-f elf'),
    ('LINKFLAGS','Sets the linker flags','-T src/system/kernel/core/processor/x86/kernel.ld -nostdlib -nostdinc -ffreestanding'),
    ('BUILDDIR','Directory to place build files in.','build'),
    ('LIBGCC','The folder containing libgcc.a.',''),
    BoolVariable('verbose','Display verbose build output.',0),
    BoolVariable('verbose_link','Display verbose linker output.',0),
    BoolVariable('warnings', 'compilation with -Wall and similiar', 1)
)

env = Environment(options = opts,tools = ['default'],ENV = os.environ)
#^-- Create a new environment object passing the options
Help(opts.GenerateHelpText(env))
#^-- Create the scons help text from the options we specified earlier
opts.Save('options.cache',env)
#^-- Save the cache file over the old one

####################################
# Compiler/Target specific settings
####################################
out = commands.getoutput(env['CXX'] + ' -v')
#^-- The old script used --dumpmachine, which isn't always present
tmp = re.match('.*?Target: ([^\n]+)',out,re.S)

if re.match('i[3456]86',tmp.group(1)) != None:
    defines +=  ['X86','X86_COMMON','LITTLE_ENDIAN']
    #^-- Should provide overloads for these...like machine=ARM_VOLITILE
elif re.match('amd64|x86[_-]64',tmp.group(1)) != None:
    defines += ['X64']
elif re.match('ppc|powerpc',tmp.group(1)) != None:
    defines += ['PPC']
elif re.match('arm',tmp.group(1)) != None:
    defines += ['ARM']
    

####################################
# Some quirks
####################################
defines += ['__UD_STANDALONE__']
#^-- Required no matter what.

if env['warnings']:
    env['CXXFLAGS'] += ' -Wall'

if env['verbose_link']:
    env['LINKFLAGS'] += ' --verbose'

####################################
# Fluff up our build messages
####################################
if not env['verbose']:
    env['CCCOMSTR']   =    '     Compiling \033[32m$TARGET\033[0m'
    env['CXXCOMSTR']  =    '     Compiling \033[32m$TARGET\033[0m'
    env['ASCOMSTR']   =    '    Assembling \033[32m$TARGET\033[0m'
    env['LINKCOMSTR'] =    '       Linking \033[32m$TARGET\033[0m'
    env['ARCOMSTR']   =    '     Archiving \033[32m$TARGET\033[0m'
    env['RANLIBCOMSTR'] =  '      Indexing \033[32m$TARGET\033[0m'
    env['NMCOMSTR']   =    '  Creating map \033[32m$TARGET\033[0m'
    env['DOCCOMSTR']  =    '   Documenting \033[32m$TARGET\033[0m'

####################################
# Setup our build options
####################################
env['CPPDEFINES'] = []
env['CPPDEFINES'] = [i for i in defines]
#^-- Stupid, I know, but again I plan on adding some preprocessing (AKA auto-options for architecutres)

####################################
# Generate Version.cc
# Exports:
## PEDIGREE_BUILDTIME
## PEDIGREE_REVISION
## PEDIGREE_FLAGS
## PEDIGREE_USER
## PEDIGREE_MACHINE
####################################
if os.path.exists('/bin/date'):
    out = commands.getoutput('/bin/date \"+%k:%M %A %e-%b-%Y\"')
    tmp = re.match('^[^\n]+',out)
    env['PEDIGREE_BUILDTIME'] = tmp.group()
else:
    env['PEDIGREE_BUILDTIME'] = '(Unknown)'

if os.path.exists('/usr/local/bin/svn'):
    #^-- Darwin
    out = commands.getoutput('/usr/local/bin/svn info .')
    tmp = re.match('.*?Revision: ([^\n]+)',out,re.S)
    env['PEDIGREE_REVISION'] = tmp.group(1)
elif os.path.exists('/usr/bin/svn'):
    #^-- Most *nix
    out = commands.getoutput('/usr/bin/svn info .')
    tmp = re.match('.*?Revision: ([^\n]+)',out,re.S)
    env['PEDIGREE_REVISION'] = tmp.group(1)
else:
    env['PEDIGREE_REVISION'] = '(Unknown)'
    
if os.path.exists('/usr/bin/whoami'):
    out = commands.getoutput('/usr/bin/whoami')
    tmp = re.match('^[^\n]+',out)
    env['PEDIGREE_USER'] = tmp.group()
else:
    env['PEDIGREE_USER'] = '(Unknown)'
    
if os.path.exists('/usr/bin/uname'):
    #^-- Darwin
    out = commands.getoutput('/usr/bin/uname')
    tmp = re.match('^[^\n]+',out)
    env['PEDIGREE_MACHINE'] = tmp.group()
elif os.path.exists('/bin/uname'):
    #^-- Most *nix
    out = commands.getoutput('/bin/uname')
    tmp = re.match('^[^\n]+',out)
    env['PEDIGREE_MACHINE'] = tmp.group()
else:
    env['PEDIGREE_MACHINE'] = '(Unknown)'
    
env['PEDIGREE_FLAGS'] = string.join(env['CPPDEFINES'],' ')
# Write the file to disk (We *assume* src/system/kernel/)
file = open('src/system/kernel/Version.cc','w')
file.writelines(['const char *g_pBuildTime = "',    env['PEDIGREE_BUILDTIME'],'";\n'])
file.writelines(['const char *g_pBuildRevision = "',env['PEDIGREE_REVISION'],'";\n'])
file.writelines(['const char *g_pBuildFlags = "',   env['PEDIGREE_FLAGS'],'";\n'])
file.writelines(['const char *g_pBuildUser = "',    env['PEDIGREE_USER'],'";\n'])
file.writelines(['const char *g_pBuildMachine = "', env['PEDIGREE_MACHINE'],'";\n'])
file.close()

####################################
# Progress through all our sub-directories
####################################
SConscript('SConscript', exports = ['env'], build_dir = env['BUILDDIR'], duplicate = 0)
