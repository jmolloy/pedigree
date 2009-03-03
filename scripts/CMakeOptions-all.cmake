#
# Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

add_option(DEBUGGER      "Enable the run-time kernel debugger." ON)
add_option(DWARF         "Enable use of the DWARF-2 debugging information format." ON)
add_option(DEBUGGER_RUN_AT_START "Boot into the debugger on startup." OFF)
add_option(DEBUGGER_QWERTY "Set the default keymap to QWERTY." ON)
add_option(DEBUGGER_QWERTZ "Set the default keymap to QWERTZ." OFF)
add_option(ECHO_CONSOLE_TO_SERIAL "Print the kernel log to serial port 1." ON)
add_option(SERIAL_IS_FILE "The serial output is not a VT100 compliant tty." ON)
add_option(VERBOSE_LINKER "Increases the verbosity of messages from the Elf and KernelElf classes." ON)
add_non_user_option(THREADS)

if(DEBUGGER)
  set(PEDIGREE_CFLAGS "${PEDIGREE_CFLAGS} -g3 -O2 -gdwarf-2")
  set(PEDIGREE_CXXFLAGS "${PEDIGREE_CXXFLAGS} -g3 -O2 -gdwarf-2")
else(DEBUGGER)
  set(PEDIGREE_CFLAGS "${PEDIGREE_CFLAGS} -g0 -O3")
  set(PEDIGREE_CXXFLAGS "${PEDIGREE_CXXFLAGS} -g0 -O3")
endif(DEBUGGER)
