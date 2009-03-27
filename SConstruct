# An attempt at porting Pedigree's build system to SCons
# - Tyler Kennedy (TkTech)

# Create a new SCons Enviornment object.
env = Environment()
# Export 'env' for our subscripts to use
Export('env')
# Load build options
env.SConscript(['BuildOptions.py'])
# Our buildtools & misc
env.SConscript(['BuildFlags.py'])

#########################################
# Setup our script defines
#########################################
## This is a piece of shit.
env['CPPDEFINES'] = []

if env['ARCHITECTURE'] == 'x86':
    env['CPPDEFINES'] += ['X86']
    env['CPPDEFINES'] += ['BITS_32']
    
if env['MACHINE'] == 'x86':
    env['CPPDEFINES'] += ['X86_COMMON']
    
if env['THREADS'] == 1:
    env['CPPDEFINES'] += ['THREADS']
    
if env['DEBUGGER'] == 1:
    env['CPPDEFINES'] += ['DEBUGGER']
    
if env['DEBUGGER_QWERTY'] == 1:
    env['CPPDEFINES'] += ['DEBUGGER_QWERTY']

if env['DEBUGGER_QWERTZ'] == 1:
    env['CPPDEFINES'] += ['DEBUGGER_QWERTZ']
    
if env['SMBIOS'] == 1:
    env['CPPDEFINES'] += ['SMBIOS']
    
if env['SMP'] == 1:
    env['CPPDEFINES'] += ['SMP']

#########################################
# Setup targets
#########################################
## Kernel
env.SConscript('src/system/kernel/SConscript',variant_dir='build')
## User
env.SConscript('src/user/SConscript',variant_dir='build')