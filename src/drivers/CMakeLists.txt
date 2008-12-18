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

set(DRIVERS ata partition)
if(X86_COMMON)
  set(X86DRIVERS vbe)
endif(X86_COMMON)
if(PPC_COMMON)
  set(PPCDRIVERS ata-specific)
endif(PPC_COMMON)

#########################################################################################

# Descend into each arch's subdirectory first.
foreach(var ${X86DRIVERS})
  add_subdirectory("x86/${var}")
endforeach(var ${X86DRIVERS})
foreach(var ${PPCDRIVERS})
  add_subdirectory("ppc/${var}")
endforeach(var ${PPCDRIVERS})
foreach(var ${DRIVERS})
  add_subdirectory("common/${var}")
endforeach(var ${DRIVERS})

# Pull all the modules together in a tarfile.
set(OBJS)
set(DRIVER_NAMES)

foreach(var ${X86DRIVERS})
  set(OBJS ${OBJS} "${PEDIGREE_BINARY_DIR}/src/drivers/x86/${var}/${var}.o")
  set(DRIVER_NAMES ${DRIVER_NAMES} "${var}.o")
endforeach(var ${X86DRIVERS})

foreach(var ${PPCDRIVERS})
  set(OBJS ${OBJS} "${PEDIGREE_BINARY_DIR}/src/drivers/ppc/${var}/${var}.o")
  set(DRIVER_NAMES ${DRIVER_NAMES} "${var}.o")
endforeach(var ${PPCDRIVERS})

foreach(var ${DRIVERS})
  set(OBJS ${OBJS} "${PEDIGREE_BINARY_DIR}/src/drivers/common/${var}/${var}.o")
  set(DRIVER_NAMES ${DRIVER_NAMES} "${var}.o")
endforeach(var ${DRIVERS})

set(DRIVER_OBJS ${OBJS} PARENT_SCOPE)
set(DRIVER_NAMES ${DRIVER_NAMES} PARENT_SCOPE)
