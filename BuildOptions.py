Import('*')

############################################
# Insert your compiler path here.

# The base path
env['COMPILER_PATH'] = '/usr/cross/bin/i586-elf'

############################################
# Sets the target machine

# Architecture family. Valid values: {x86, ppc, mips, arm}
env['ARCH_FAMILY']      = 'x86'

# Architecture subtype. Valid values: {ia32, amd64}    if ARCH_FAMILY = x86
#                                     {ppc32}          if ARCH_FAMILY = ppc
#                                     {mipsel, mipseb} if ARCH_FAMILY = mips
#                                     {arm926e}        if ARCH_FAMILY = arm
env['ARCHITECTURE']     = 'ia32'

# Machine type. Valid values: {pc}                    if ARCH_FAMILY = x86
#                             {mac}                   if ARCH_FAMILY = ppc
#                             {malta, bonito64}       if ARCH_FAMILY = mips
#                             {integrator, versatile} if ARCH_FAMILY = arm
env['MACHINE']          = 'pc'

###########################################
# Compile options

###########################################
# Common flags

env['THREADS']                = 1 # Enable threading
env['DEBUGGER']               = 1 # Enable the debugger
env['DEBUGGER_QWERTY']        = 1 # Enable the QWERTY keymap
env['DEBUGGER_QWERTZ']        = 0 # Enable the QWERTZ keymap
env['DWARF']                  = 0 # Enable use of DWARF debugging information by the debugger.
env['DEBUGGER_RUN_AT_START']  = 0 # Start the debugger as soon as possible (use for debugging boot errors)
env['ECHO_CONSOLE_TO_SERIAL'] = 1 # Echo all log messages to serial port #1.
env['SERIAL_IS_FILE']         = 1 # The serial output is not a VT100 compliant tty.

###########################################
# Flags applicable to x86 only

env['SMBIOS']           = 1 # Enable SMBIOS
env['SMP']              = 0 # Enable SMP
env['ACPI']             = 0 # Enable ACPI
env['APIC']             = 0 # Enable APIC
env['MULTIPROCESSOR']   = 0 # Enable Multiprocessor

