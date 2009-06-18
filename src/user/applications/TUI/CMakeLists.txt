#
# Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

set(CMAKE_C_FLAGS "-fno-builtin -fno-stack-protector -U__svr4__ -nostdlib")
include_directories("${PEDIGREE_BINARY_DIR}/include-posix" "${PEDIGREE_SOURCE_DIR}/src/system/modules/TUI/" "${PEDIGREE_SOURCE_DIR}/src/user/applications/TUI/include")

set_source_files_properties(${PEDIGREE_BINARY_DIR}/src/subsys/posix/crt0.o PROPERTIES GENERATED 1)
add_executable(TUI main.cc Font.cc Png.cc colourSpace.cc Header.cc _Xterm.cc Terminal.cc environment.cc ${PEDIGREE_BINARY_DIR}/src/subsys/posix/crt0.o)
set_target_properties(TUI PROPERTIES
                              LINK_FLAGS "-L ${PEDIGREE_BINARY_DIR}")
target_link_libraries(TUI c)
target_link_libraries(TUI m)
target_link_libraries(TUI gcc)
target_link_libraries(TUI "${PEDIGREE_SOURCE_DIR}/src/user/applications/TUI/libfreetype-i686.a" "${PEDIGREE_SOURCE_DIR}/src/user/applications/TUI/libpng-i686.a" "${PEDIGREE_SOURCE_DIR}/src/user/applications/TUI/libz-i686.a")
add_dependencies(TUI posix-usermode crt0.o)
