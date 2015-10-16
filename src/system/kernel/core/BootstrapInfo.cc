/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <compiler.h>
#include <processor/types.h>
#include <utilities/String.h>

#include <BootstrapInfo.h>

#ifndef PPC_COMMON

struct MemoryMapEntry_t
{
    uint32_t size;
    uint64_t address;
    uint64_t length;
    uint32_t type;
} PACKED;

BootstrapStruct_t::BootstrapStruct_t()
{
    flags = 0;
}

bool BootstrapStruct_t::isInitrdLoaded() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return (mods_count != 0);
    else
        return false;
}

uint8_t *BootstrapStruct_t::getInitrdAddress() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return reinterpret_cast<uint8_t *>(getModuleArray()[0].base);
    else
        return 0;
}

size_t BootstrapStruct_t::getInitrdSize() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return getModuleArray()[0].end - getModuleArray()[0].base;
    else
        return 0;
}

bool BootstrapStruct_t::isDatabaseLoaded() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return (mods_count > 1);
    else
        return 0;
}

uint8_t *BootstrapStruct_t::getDatabaseAddress() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return reinterpret_cast<uint8_t *>(getModuleArray()[1].base);
    else
        return 0;
}

size_t BootstrapStruct_t::getDatabaseSize() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return getModuleArray()[1].end - getModuleArray()[1].base;
    else
        return 0;
}

char *BootstrapStruct_t::getCommandLine() const
{
    if (flags & MULTIBOOT_FLAG_CMDLINE)
        return reinterpret_cast<char *>(cmdline);
    else
        return 0;
}

size_t BootstrapStruct_t::getSectionHeaderCount() const
{
    if (flags & MULTIBOOT_FLAG_ELF)
        return num;
    else
        return 0;
}

size_t BootstrapStruct_t::getSectionHeaderEntrySize() const
{
    if (flags & MULTIBOOT_FLAG_ELF)
        return size;
    else
        return 0;
}

size_t BootstrapStruct_t::getSectionHeaderStringTableIndex() const
{
    if (flags & MULTIBOOT_FLAG_ELF)
        return shndx;
    else
        return 0;
}

uintptr_t BootstrapStruct_t::getSectionHeaders() const
{
    if (flags & MULTIBOOT_FLAG_ELF)
        return addr;
    else
        return 0;
}

void *BootstrapStruct_t::getMemoryMap() const
{
    if (flags & MULTIBOOT_FLAG_MMAP)
        return reinterpret_cast<void *>(mmap_addr);
    else
        return 0;
}

uint64_t BootstrapStruct_t::getMemoryMapEntryAddress(void *opaque) const
{
    if (!opaque)
        return 0;

    MemoryMapEntry_t *entry = reinterpret_cast<MemoryMapEntry_t *>(opaque);
    return entry->address;
}

uint64_t BootstrapStruct_t::getMemoryMapEntryLength(void *opaque) const
{
    if (!opaque)
        return 0;

    MemoryMapEntry_t *entry = reinterpret_cast<MemoryMapEntry_t *>(opaque);
    return entry->length;
}

uint32_t BootstrapStruct_t::getMemoryMapEntryType(void *opaque) const
{
    if (!opaque)
        return 0;

    MemoryMapEntry_t *entry = reinterpret_cast<MemoryMapEntry_t *>(opaque);
    return entry->type;
}

void *BootstrapStruct_t::nextMemoryMapEntry(void *opaque) const
{
    if (!opaque)
        return 0;

    MemoryMapEntry_t *entry = reinterpret_cast<MemoryMapEntry_t *>(opaque);
    uintptr_t entry_addr = reinterpret_cast<uintptr_t>(opaque);
    void *new_opaque = reinterpret_cast<void *>(entry_addr + entry->size + 4);

    if (reinterpret_cast<uintptr_t>(new_opaque) >= (mmap_addr + mmap_length))
        return 0;
    else
        return new_opaque;
}

size_t BootstrapStruct_t::getModuleCount() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return mods_count;
    else
        return 0;
}

void *BootstrapStruct_t::getModuleBase() const
{
    if (flags & MULTIBOOT_FLAG_MODS)
        return reinterpret_cast<void *>(mods_addr);
    else
        return 0;
}

#endif
