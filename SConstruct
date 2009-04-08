####################################
# SCons build system for Pedigree
## Tyler Kennedy (AKA Linuxhq AKA TkTech)
####################################

import os

opts = Variables('options.cache')
opts.AddVariables(
    ('CC','Sets the C compiler to use.'),
    ('CXX','Sets the C++ compiler to use.'),
    ('AS','Sets the assembler to use.'),
    ('LINK','Sets the linker to use.'),
    ('CFLAGS','Sets the C compiler flags.','-march=i486 -fno-builtin -fno-stack-protector -nostdlib -m32 -g0 -O3'),
    ('CXXFLAGS','Sets the C++ compiler flags.','-march=i486 -fno-builtin -fno-stack-protector -nostdlib -m32 -g0 -O3 -Weffc++ -Wall -Wold-style-cast -Wno-long-long -fno-rtti -fno-exceptions'),
    ('ASFLAGS','Sets the assembler flags.','-f elf'),
    ('LINKFLAGS','Sets the linker flags','-nostdlib -nostdinc'),
    ('BUILDDIR','Directory to place build files in.','build'),
    BoolVariable('verbose','Display verbose build output.',False),
    BoolVariable('warnings', 'compilation with -Wall and similiar', 1)
)

env = Environment(options = opts,tools = ['default'],ENV = os.environ) # Create a new environment object passing the options
Help(opts.GenerateHelpText(env)) # Create the scons help text from the options we specified earlier
opts.Save('options.cache',env)

####################################
# Fluff up our build messages
####################################
if not env['verbose']:
    env['CCCOMSTR'] =      '     Compiling \033[32m$TARGET\033[0m'
    env['ASPPCOMSTR'] =    '    Assembling \033[32m$TARGET\033[0m'
    env['LINKCOMSTR'] =    '       Linking \033[32m$TARGET\033[0m'
    env['ARCOMSTR'] =      '     Archiving \033[32m$TARGET\033[0m'
    env['RANLIBCOMSTR'] =  '      Indexing \033[32m$TARGET\033[0m'
    env['NMCOMSTR'] =      '  Creating map \033[32m$TARGET\033[0m'
    env['DOCCOMSTR'] =     '   Documenting \033[32m$TARGET\033[0m'

####################################
# Progress through all our sub-directories
####################################
SConscript('SConscript',
           exports = ['env'],
           build_dir = env['BUILDDIR'],
           duplicate = 0) # duplicate = 0 stops it from copying the sources to the build dir