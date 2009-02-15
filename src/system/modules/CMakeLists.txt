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

set(MODULES)
set(MODULES ${MODULES} vfs ext2 fat init console TUI linker)
# network

#########################################################################################

# Descend into each module's subdirectory first.
foreach(var ${MODULES})
  add_subdirectory(${var})
endforeach(var ${MODULES})

# Pull all the modules together in a tarfile.
set(OBJS)
set(MODULE_NAMES)
foreach(var ${MODULES})
  set(OBJS ${OBJS} "${PEDIGREE_BINARY_DIR}/src/system/modules/${var}/${var}.o")
  set(MODULE_NAMES ${MODULE_NAMES} "${var}.o")
endforeach(var ${MODULES})

set(MODULE_OBJS ${OBJS} PARENT_SCOPE)
set(MODULE_NAMES ${MODULE_NAMES} PARENT_SCOPE)
