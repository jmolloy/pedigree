# An attempt at porting Pedigree's build system to SCons
# - Tyler Kennedy (TkTech)

# Create a new SCons Enviornment object.
env = Environment()
#########################################
# Setup our environment varibles
#########################################
# The base path
env['COMPILER_PATH'] = '/usr/cross/bin/i586-elf'
# C Compiler
env['CC'] = '$COMPILER_PATH-gcc'
# C++ Compiler
env['CXX'] = '$COMPILER_PATH-g++'
# Assembler
env['AS'] = 'nasm'
# Linker
env['LINK'] = '$COMPILER_PATH-ld'
#########################################
# Flags
#########################################
# C/C++ shared flags
env['SHARED_FLAGS'] = '-march=i486 -fno-builtin -fno-stack-protector -nostdlib -m32 -g0 -O3'
# C flags
env['CFLAGS'] = '$SHARED_FLAGS'
# C++ flags
env['CXXFLAGS'] = '$SHARED_FLAGS -Weffc++ -Wall -Wold-style-cast -Wno-long-long -fno-rtti -fno-exceptions'
# Assembler flags
env['ASFLAGS'] = '-f elf'
# Linker flags
env['LINKFLAGS'] = '-T src/system/kernel/core/processor/x86/kernel.ld -nostdlib -nostdinc'
#########################################
# Source defines
#########################################
env['CPPDEFINES'] = ['X86','X86_COMMON','BITS_32','THREADS','MULTIPROCESSOR','DEBUGGER','APIC','ACPI','DEBUGGER_QWERTY','SMBIOS','SMP']
#########################################
# Setup targets
#########################################
### Target 'KERNEL'
K_DIR = 'src/system/kernel/'
# Can't just use Glob('src/system/kernel/*.cc') because *Someone* put MIPS and ARM code in the directory -_-
K_SOURCES  = [K_DIR+'Log.cc',K_DIR+'Archive.cc',K_DIR+'Spinlock.cc']
K_SOURCES += [Glob(K_DIR + 'core/*.cc')]
K_SOURCES += [Glob(K_DIR + 'core/processor/x86/*.cc')]
K_SOURCES += [Glob(K_DIR + 'core/lib/*.c'),Glob(K_DIR + 'core/lib/*.cc')]
K_SOURCES += [Glob(K_DIR + 'core/utilities/*.cc')]
K_SOURCES += [Glob(K_DIR + 'debugger/*.cc')]
K_SOURCES += [Glob(K_DIR + 'linker/*.cc')]
K_SOURCES += [Glob(K_DIR + 'core/process/*.cc')]
K_SOURCES += [Glob(K_DIR + 'machine/x86_common/*.cc')]

# Include directories...
K_INCLUDES  = ['src/system/include']
K_INCLUDES += ['src/system/include/linker/']
K_INCLUDES += ['src/system/include/process/']
K_INCLUDES += [K_DIR]
K_INCLUDES += [K_DIR + 'core/']
K_INCLUDES += [K_DIR + 'core/processor/x86/']
K_INCLUDES += [K_DIR + 'core/process/']
K_INCLUDES += [K_DIR + 'core/lib/']
K_INCLUDES += [K_DIR + 'linker/']
K_INCLUDES += [K_DIR + 'debugger/']
K_INCLUDES += [K_DIR + 'debugger/commands/']
K_INCLUDES += [K_DIR + 'machine/x86_common/']
# Add it as a target
env.Program(target = 'Kernel' , source = K_SOURCES, CPPPATH = K_INCLUDES)
### End 'KERNEL'