#
# Copyright (c) 2008 James Molloy, James Pritchett, JÃ¶rg PfÃ¤hler, Matthew Iselin
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

set(CMAKE_C_FLAGS "-fno-builtin -fno-stack-protector -U__svr4__")
set_source_files_properties(${PEDIGREE_BINARY_DIR}/src/subsys/posix/crt0.o PROPERTIES GENERATED 1)
add_executable(net-test net-test.c ${PEDIGREE_BINARY_DIR}/src/subsys/posix/crt0.o)
include_directories("${PEDIGREE_BINARY_DIR}/include-posix")
set_target_properties(net-test PROPERTIES
                            LINK_FLAGS "-L ${PEDIGREE_BINARY_DIR}")
target_link_libraries(net-test c)
target_link_libraries(net-test gcc)
target_link_libraries(net-test m)
add_dependencies(net-test posix-usermode crt0.o)
