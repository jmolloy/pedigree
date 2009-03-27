Import('*')
#############################
# Sets the target machine
#############################
env['ARCH_FAMILY']      = 'x86'
env['ARCHITECTURE']     = 'x86'
env['MACHINE']          = 'x86'
#############################
# Compile options
## At present, some of these options may cause broken
## builds or won't even compile.
#############################
env['THREADS']          = 1 # Enable threading
env['DEBUGGER']         = 1 # Enable the debugger
env['DEBUGGER_QWERTY']  = 1 # Enable the QWERTY keymap
env['DEBUGGER_QWERTZ']  = 0 # Enable the QWERTZ keymap
env['SMBIOS']           = 1 # Enable SMBIOS
env['SMP']              = 0 # Enable SMP
env['ACPI']             = 0 # Enable ACPI
env['APIC']             = 0 # Enable APIC
env['MULTIPROCESSOR']   = 0 # Enable Multiprocessor
