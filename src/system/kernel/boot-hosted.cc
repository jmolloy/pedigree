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

#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#include <elf.h>

#include "BootstrapInfo.h"

#pragma GCC diagnostic warning "-fpermissive"

extern "C" void _main(BootstrapStruct_t &bs);

static uint32_t ptr_to_u32(void *p)
{
    uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
    assert((ptr >> 32ULL) == 0);
    return static_cast<uint32_t>(ptr) & 0xFFFFFFFFUL;
}

extern "C"
int main(int argc, char *argv[])
{
    struct stat st;
    int r = 0;
    int s = 0;
    int initrd = -1;
    int configdb = -1;
    int kernel = -1;
    int diskimage = -1;
    void *initrd_mapping = MAP_FAILED;
    void *configdb_mapping = MAP_FAILED;
    void *kernel_mapping = MAP_FAILED;
    void *diskimage_mapping = MAP_FAILED;
    uint32_t *module_region = (uint32_t *) MAP_FAILED;
    size_t initrd_length = 0;
    size_t configdb_length = 0;
    size_t kernel_length = 0;
    size_t diskimage_length = 0;
    Elf64_Ehdr *ehdr = 0;
    Elf64_Shdr *shdrs = 0;
    BootstrapStruct_t bs;
    memset(&bs, 0, sizeof(bs));

    if(argc < 3)
    {
        fprintf(stderr, "Usage: kernel initrd config_database [diskimage]\n");
        goto fail;
    }

    fprintf(stderr, "Pedigree is starting...\n");

    // Load initrd and config database into RAM.
    initrd = open(argv[1], O_RDONLY);
    if(initrd < 0)
    {
        fprintf(stderr, "Can't open initrd: %s\n", strerror(errno));
        goto fail;
    }
    configdb = open(argv[2], O_RDONLY);
    if(configdb < 0)
    {
        fprintf(stderr, "Can't open config database: %s\n", strerror(errno));
        goto fail;
    }

    // Open ourselves to find section headers.
    kernel = open(argv[0], O_RDONLY);
    if(kernel < 0)
    {
        fprintf(stderr, "Can't open kernel: %s\n", strerror(errno));
        goto fail;
    }

    // Load initrd and configuration database.
    r = fstat(initrd, &st);
    if(r != 0)
    {
        fprintf(stderr, "Can't stat initrd: %s\n", strerror(errno));
        goto fail;
    }
    initrd_length = st.st_size;
    initrd_mapping = mmap(0, initrd_length, PROT_READ, MAP_PRIVATE | MAP_32BIT, initrd, 0);
    if(initrd_mapping == MAP_FAILED)
    {
        fprintf(stderr, "Can't map initrd: %s\n", strerror(errno));
        goto fail;
    }

    r = fstat(configdb, &st);
    if(r != 0)
    {
        fprintf(stderr, "Can't stat config database: %s\n", strerror(errno));
        goto fail;
    }
    configdb_length = st.st_size;
    configdb_mapping = mmap(0, configdb_length, PROT_READ, MAP_PRIVATE | MAP_32BIT, configdb, 0);
    if(configdb_mapping == MAP_FAILED)
    {
        fprintf(stderr, "Can't map config database: %s\n", strerror(errno));
        goto fail;
    }

    r = fstat(kernel, &st);
    if(r != 0)
    {
        fprintf(stderr, "Can't stat kernel: %s\n", strerror(errno));
        goto fail;
    }
    kernel_length = st.st_size;
    kernel_mapping = mmap(0, kernel_length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_32BIT, kernel, 0);
    if(kernel_mapping == MAP_FAILED)
    {
        fprintf(stderr, "Can't map kernel: %s\n", strerror(errno));
        goto fail;
    }

    // Make the module locations available to the kernel.
    module_region = (uint32_t *) mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_32BIT, -1, 0);
    if(module_region == MAP_FAILED)
    {
        fprintf(stderr, "Can't map module information region: %s\n", strerror(errno));
        goto fail;
    }
    memset(module_region, 0, 0x1000);

    // initrd
    module_region[0] = ptr_to_u32(initrd_mapping);
    module_region[1] = ptr_to_u32(initrd_mapping) + initrd_length;

    // config database
    module_region[4] = ptr_to_u32(configdb_mapping);
    module_region[5] = ptr_to_u32(configdb_mapping) + configdb_length;

    bs.mods_addr = ptr_to_u32(module_region);
    bs.mods_count = 2;

    if (argc > 3)
    {
        diskimage = open(argv[3], O_RDWR);
        if(diskimage < 0)
        {
            fprintf(stderr, "Can't open disk image: %s\n", strerror(errno));
            goto fail;
        }

        r = fstat(diskimage, &st);
        if(r != 0)
        {
            fprintf(stderr, "Can't stat disk image: %s\n", strerror(errno));
            goto fail;
        }

        diskimage_length = st.st_size;
        diskimage_mapping = mmap(0, diskimage_length, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_NORESERVE, diskimage, 0);
        if(diskimage_mapping == MAP_FAILED)
        {
            fprintf(stderr, "Can't map disk image: %s\n", strerror(errno));
            goto fail;
        }

        // Add to the multiboot info.
        bs.mods_count++;
        *((uint64_t*) &module_region[8]) = reinterpret_cast<uintptr_t>(diskimage_mapping);
        *((uint64_t*) &module_region[10]) = reinterpret_cast<uintptr_t>(diskimage_mapping) + diskimage_length;
    }

    // Load ELF header to add ELF information.
    ehdr = reinterpret_cast<Elf64_Ehdr*>(kernel_mapping);
    bs.shndx = ehdr->e_shstrndx;
    bs.num = ehdr->e_shnum;
    bs.size = ehdr->e_shentsize;
    bs.addr = ptr_to_u32(kernel_mapping) + ehdr->e_shoff;

    // Fix up section headers with no addresses.
    shdrs = reinterpret_cast<Elf64_Shdr *>(bs.addr);
    for(uint32_t i = 1; i < bs.num; ++i)
    {
        if(shdrs[i].sh_addr)
            continue;

        shdrs[i].sh_addr = shdrs[i].sh_offset + reinterpret_cast<uintptr_t>(kernel_mapping);
    }

    // Kernel uses flags to know what it can and can't use.
    bs.flags |= MULTIBOOT_FLAG_MODS | MULTIBOOT_FLAG_ELF;
    _main(bs);

    goto cleanup;
fail:
    s = 1;
cleanup:
    if(module_region != MAP_FAILED)
        munmap(module_region, 0x1000);
    if(diskimage_mapping != MAP_FAILED)
        munmap(diskimage_mapping, diskimage_length);
    if(kernel_mapping != MAP_FAILED)
        munmap(kernel_mapping, kernel_length);
    if(configdb_mapping != MAP_FAILED)
        munmap(configdb_mapping, kernel_length);
    if(initrd_mapping != MAP_FAILED)
        munmap(initrd_mapping, initrd_length);
    close(diskimage);
    close(kernel);
    close(configdb);
    close(initrd);
    return s;
}
