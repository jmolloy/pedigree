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

#
# NOTE: To ensure proper scoping, the add_subdirectory calls must come BEFORE
#       any add_kernel_src's in this file. Additionally, at least one add_kernel_src
#       MUST be added. If no files are to be added, just do "add_kernel_src(directory)".
#

if(MIPS_AU1500)
  add_subdirectory(au1500)
endif(MIPS_AU1500)

if(MIPS_MALTA)
  add_subdirectory(malta)
endif(MIPS_MALTA)

if(MIPS_COMMON)
  add_subdirectory(mips_common)
endif(MIPS_COMMON)

if(X86_COMMON)
  add_subdirectory(x86_common)
endif(X86_COMMON)

if(ARM_VERSATILE)
  add_subdirectory(arm_versatile)
endif(ARM_VERSATILE)

if(ARM_COMMON)
  add_subdirectory(arm_common)
endif(ARM_COMMON)

if(PPC_MAC)
  add_subdirectory(mac)
endif(PPC_MAC)

if(PPC_COMMON)
  add_subdirectory(ppc_common)
  add_subdirectory(openfirmware)
endif(PPC_COMMON)

# This is required to push any changes the add_subdirectory commands may have made up to parent scope.
add_kernel_src(machine Device.cc Network.cc)
