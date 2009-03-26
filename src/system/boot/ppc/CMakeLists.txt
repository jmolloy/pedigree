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

#########################################################################################
# Sources list.
set(BOOT_SRC boot.s main.cc prom.cc Elf32.cc)

#########################################################################################
# Script begin.

include_directories(.)

# Add an extra target to create the bootloader.
add_custom_target(autogen.h ALL perl ${PEDIGREE_SOURCE_DIR}/src/system/boot/ppc/mkheader.pl ${PEDIGREE_BINARY_DIR}/src/system/kernel/kernel
                  DEPENDS kernel
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/system/boot/ppc)

# Add an extra target to create the bootloader.
add_custom_target(autogen_initrd.h ALL cp ${PEDIGREE_BINARY_DIR}/initrd.tar /tmp/initrd.tar.gz
                                   COMMAND gunzip -f /tmp/initrd.tar.gz
				   COMMAND perl ${PEDIGREE_SOURCE_DIR}/src/system/boot/ppc/mkheader.pl /tmp/initrd.tar
                  WORKING_DIRECTORY ${PEDIGREE_SOURCE_DIR}/src/system/boot/ppc)

add_executable(bootloader ${BOOT_SRC})
add_dependencies(bootloader autogen.h autogen_initrd.h)
set_target_properties(bootloader PROPERTIES LINK_FLAGS
                      "-T ${PEDIGREE_SOURCE_DIR}/src/system/boot/ppc/link-ppc.ld")
