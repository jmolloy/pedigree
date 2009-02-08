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
# The project name and CMake version.

project(PEDIGREE C CXX ASM-ATT)
cmake_minimum_required(VERSION 2.6)

option(COLORGCC "Use the colorgcc script to colourise the compiler output." ON)

#########################################################################################
# Set the compiler location, if CROSS is set.

if (DEFINED CROSS)
  set(C_COMPILER "${CROSS}gcc")
  set(CXX_COMPILER "${CROSS}g++")
  set(ASM-ATT_COMPILER "${CROSS}as")
else (CROSS)
  set(C_COMPILER ${CMAKE_C_COMPILER})
  set(CXX_COMPILER ${CMAKE_CXX_COMPILER})
  set(ASM-ATT_COMPILER ${CMAKE_ASM-ATT_COMPILER})
endif (DEFINED CROSS)

# Attempt to detect compiler type.
execute_process(COMMAND ${CXX_COMPILER} "-dumpmachine"
                OUTPUT_VARIABLE MACHINE_SPEC)
string(REGEX MATCH "([^-]+)" ARCH_SPEC "${MACHINE_SPEC}")

if(ARCH_SPEC MATCHES "i[3456]86")
  set(ARCH "x86")
  get_filename_component(C_COMPILER_DIR ${C_COMPILER} PATH)
  find_program(NASM NAMES nasm HINTS ${C_COMPILER_DIR} PATHS ${C_COMPILER_DIR})
  if(NASM STREQUAL "NASM-NOTFOUND")
    message(FATAL_ERROR "`nasm' not found.")
  endif(NASM STREQUAL "NASM-NOTFOUND")
  set(ASM-ATT_COMPILER "${NASM}")
elseif(ARCH_SPEC MATCHES "amd64|x86[_-]64")
  set(ARCH "x64")
  get_filename_component(C_COMPILER_DIR ${C_COMPILER} PATH)
  find_program(NASM NAMES nasm HINTS ${C_COMPILER_DIR} PATHS ${C_COMPILER_DIR})
  if(NASM STREQUAL "NASM-NOTFOUND")
    message(FATAL_ERROR "`nasm' not found.")
  endif(NASM STREQUAL "NASM-NOTFOUND")
  set(ASM-ATT_COMPILER "${NASM}")
elseif(ARCH_SPEC MATCHES "ppc|powerpc")
  set(ARCH "ppc")
elseif(ARCH_SPEC MATCHES "mips(eb|be)")
  message(FATAL_ERROR "Big-endian MIPS is not supported at this time.")
elseif(ARCH_SPEC MATCHES "mips(el|le)")
  set(ARCH "mips")
elseif(ARCH_SPEC MATCHES "arm")
  set(ARCH "arm")
endif(ARCH_SPEC MATCHES "i[3456]86")

message(STATUS "Compiler machine type '${ARCH_SPEC}' - compiling for architecture '${ARCH}'")

# If we're using COLORGCC, create the symlinks to the compilers.
if (COLORGCC)
  get_filename_component(COMPILER_DIR ${C_COMPILER} PATH)
  get_filename_component(C_COMPILER_STUB ${C_COMPILER} NAME)
  get_filename_component(CXX_COMPILER_STUB ${CXX_COMPILER} NAME)
  # Make sure any stale links are deleted.
  file(REMOVE "${PEDIGREE_BINARY_DIR}/${C_COMPILER_STUB}")
  file(REMOVE "${PEDIGREE_BINARY_DIR}/${CXX_COMPILER_STUB}")
  file(REMOVE "${PEDIGREE_BINARY_DIR}/compiler-dir")
  # And create the new ones.
  execute_process(COMMAND "ln" "-s" "${PEDIGREE_SOURCE_DIR}/scripts/colorgcc.pl" "${PEDIGREE_BINARY_DIR}/${C_COMPILER_STUB}")
  execute_process(COMMAND "ln" "-s" "${PEDIGREE_SOURCE_DIR}/scripts/colorgcc.pl" "${PEDIGREE_BINARY_DIR}/${CXX_COMPILER_STUB}")
  execute_process(COMMAND "ln" "-s" "${COMPILER_DIR}" "${PEDIGREE_BINARY_DIR}/compiler-dir")
  # Now assign the compiler variables
  set(CMAKE_C_COMPILER "${PEDIGREE_BINARY_DIR}/${C_COMPILER_STUB}")
  set(CMAKE_CXX_COMPILER "${PEDIGREE_BINARY_DIR}/${CXX_COMPILER_STUB}")
  set(CMAKE_ASM-ATT_COMPILER "${ASM-ATT_COMPILER}")
else (COLORGCC)
  set(CMAKE_C_COMPILER "${C_COMPILER}")
  set(CMAKE_CXX_COMPILER "${CXX_COMPILER}")
  set(CMAKE_ASM-ATT_COMPILER "${ASM-ATT_COMPILER}")
endif (COLORGCC)

#########################################################################################
# Architecture-specific options.
function(add_option var text default)
  option(${var} ${text} ${default})
  if(${var})
    add_definitions("-D${var}=1")
  else(${var})
    remove_definitions("-D${var}=1")
  endif(${var})
endfunction(add_option)

function(add_non_user_options var)
  foreach(e ${ARGN})
    set(${e} ON CACHE INTERNAL "bleh" FORCE)
    add_definitions("-D${e}=1")
  endforeach(e)
endfunction(add_non_user_options)


function(add_non_user_option e)
  set(${e} ON CACHE INTERNAL "bleh" FORCE)
  add_definitions("-D${e}=1")
endfunction(add_non_user_option)

include("${PEDIGREE_SOURCE_DIR}/scripts/CMakeOptions-${ARCH}.cmake")
include("${PEDIGREE_SOURCE_DIR}/scripts/CMakeOptions-all.cmake")

#########################################################################################
# Supplemental CFLAGS setup.

# Remove the default -rdynamic option from the linker flags.
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
# Remove the default -fPIC option from the linker flags.
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
# Remove the default -rdynamic option from the linker flags.
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
# Remove the default -fPIC option from the linker flags.
set(CMAKE_SHARED_LIBRARY_C_FLAGS)

set(CMAKE_CXX_FLAGS ${PEDIGREE_CXXFLAGS})
set(CMAKE_C_FLAGS ${PEDIGREE_CFLAGS})
set(CMAKE_ASM-ATT_FLAGS ${PEDIGREE_ASFLAGS})

#########################################################################################
# Create version.cc file.
if(EXISTS "/bin/date")
  execute_process(COMMAND "/bin/date" "+%k:%M %A %e-%b-%Y"
                  OUTPUT_VARIABLE BUILDTIME)
  string(REGEX MATCH "^[^\n]+" PEDIGREE_BUILDTIME ${BUILDTIME})
endif(EXISTS "/bin/date")
if(EXISTS "/usr/bin/svn")
  execute_process(COMMAND "svn" "info" "${PEDIGREE_SOURCE_DIR}"
                  OUTPUT_VARIABLE REVISION)
  string(REGEX MATCH "Revision: ([^\n]+)" REVISION ${REVISION})
  set(PEDIGREE_REVISION ${CMAKE_MATCH_1})
endif(EXISTS "/usr/bin/svn")

get_property(FLAGS DIRECTORY PROPERTY COMPILE_DEFINITIONS)
string(REPLACE "=1" " " PEDIGREE_FLAGS ${FLAGS})

if(EXISTS "/usr/bin/whoami")
  execute_process(COMMAND "/usr/bin/whoami"
                  OUTPUT_VARIABLE WHOAMI)
  string(REGEX MATCH "^[^\n]+" PEDIGREE_USER ${WHOAMI})
endif(EXISTS "/usr/bin/whoami")

if(EXISTS "/bin/uname")
  execute_process(COMMAND "/bin/uname" "-n"
                  OUTPUT_VARIABLE UNAME)
  string(REGEX MATCH "^[^\n]+" PEDIGREE_MACHINE ${UNAME})
endif(EXISTS "/bin/uname")

configure_file("${PEDIGREE_SOURCE_DIR}/scripts/Version.cc.in" "${PEDIGREE_SOURCE_DIR}/src/system/kernel/Version.cc")

#########################################################################################
# Main targets.

# Recurse into the Kernel directory first.
add_subdirectory(src/system/kernel)

# Recurse into the Modules directory.
add_subdirectory(src/drivers)
add_subdirectory(src/system/modules)

# Add the subsystems.
add_subdirectory(src/subsys)

# All the object files go in the /modules directory.
install(FILES ${DRIVER_OBJS} ${MODULE_OBJS} ${SUBSYS_OBJS}
        DESTINATION modules)

add_custom_target(initrd ALL tar --absolute-names --transform='s,/.*/,,' -czf initrd.tar ${DRIVER_OBJS} ${MODULE_OBJS} ${SUBSYS_OBJS}
                   DEPENDS ${DRIVER_NAMES} ${MODULE_NAMES} ${SUBSYS_NAMES}
                   WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR})
# The initrd.tar file also gets installed - in /.
install(FILES ${PEDIGREE_BINARY_DIR}/initrd.tar
        DESTINATION "kernel/")


add_dependencies(initrd kernel)

add_subdirectory(src/user)

# Create a target used for creating the image files.
add_custom_target(create_images ALL bash ../scripts/create_images.sh ${HDD_FILES}
                  DEPENDS kernel ${BOOTLOADER_BUILT} ${HDD_TARGETS}
                  WORKING_DIRECTORY ${PEDIGREE_BINARY_DIR})

add_dependencies(create_images initrd bash nano)

include(CPack)
