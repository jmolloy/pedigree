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
set(BOOT_SRC main.cc Elf32.cc arm_support.s autogen.h)

#########################################################################################
# Script begin.

include_directories(.)

# Add an extra target to create the bootloader.
add_custom_command(OUTPUT autogen.h
                   COMMAND perl ${PEDIGREE_SOURCE_DIR}/src/system/boot/arm/mkheader.pl)

add_executable(bootloader ${BOOT_SRC})
add_dependencies(bootloader kernel)
set_target_properties(bootloader PROPERTIES LINK_FLAGS
                      "-T ${PEDIGREE_SOURCE_DIR}/src/system/boot/arm/link-arm.ld")
