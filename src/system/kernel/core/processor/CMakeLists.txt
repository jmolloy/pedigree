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

if(MIPS32)
  add_subdirectory(mips32)
endif(MIPS32)

if(MIPS64)
  add_subdirectory(mips64)
endif(MIPS64)

if(MIPS_COMMON)
  add_subdirectory(mips_common)
endif(MIPS_COMMON)

if(X64)
  add_subdirectory(x64)
endif(X64)

if(X86)
  add_subdirectory(x86)
endif(X86)

if(X86_COMMON)
  add_subdirectory(x86_common)
endif(X86_COMMON)

add_kernel_src(core/processor
               IoPort.cc IoPortManager.cc Processor.cc StackFrameBase.cc VirtualAddressSpace.cc)
add_kernel_src_asm(core/processor)