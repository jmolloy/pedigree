/*
 * 
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
#include <stdlib.h>
#include <string.h>
#include "elf.h"

#define MODULE_FMT "root»/system/modules/%s.o"

extern void pedigree_module_load(char *file);
extern int pedigree_module_is_loaded(char *name);
int load_module(char *name);

int load_module(char *name)
{
    // Make the file name
    char *file = (char*)malloc(strlen(name) + strlen(MODULE_FMT) - 1); // -1 because we have -2 (%s) and +1 (\0)
    sprintf(file, MODULE_FMT, name);
    // Try to open the file
    FILE *fp = fopen(file, "r");
    if(fp == NULL)
    {
        printf("Module file %s not found!\n", file);
        return -1;
    }
    fseek(fp,0,SEEK_END);
    size_t len = ftell(fp);
    fseek(fp,0,SEEK_SET);
    // Read the contents of the file
    uint8_t *buffer = (uint8_t *)malloc(len);
    fread(buffer, len, 1, fp);
    fclose(fp);
    // Create the elf structure
    Elf_t *elf = elf_create(buffer, len);
    if(!elf)
        return -1;
    // Grab the name and dependencies symbols
    ElfSymbol_t *name_sym = elf_get_symbol(elf, "g_pModuleName"), *deps_sym = elf_get_symbol(elf, "g_pDepends");
    if(!name_sym)
    {
        printf("g_pModuleName symbol not found!\n");
        return -1;
    }
    if(!deps_sym)
    {
        printf("g_pDepends symbols not found!\n");
        return -1;
    }
    char *modname = (char*)elf_relptr(elf, name_sym, *SYM_PTR(elf, name_sym));
    if(pedigree_module_is_loaded(modname))
    {
        printf("Module %s is already loaded!\n", modname);
        return -1;
    }
    printf("Module %s being loaded...\n", modname);
    uint32_t *deps = SYM_PTR(elf, deps_sym);
    for(size_t i = 0; deps[i]; i++)
    {
        char *depname = (char*)elf_relptr(elf, deps_sym, deps[i]);
        if(pedigree_module_is_loaded(depname))
            continue;
        if(!load_module(depname) && pedigree_module_is_loaded(depname))
            continue;
        printf("Dependency %s of %s couldn't been loaded!\n", depname, modname);
        return -1;
    }
    // Finally load the module
    pedigree_module_load(file);
    if(pedigree_module_is_loaded(modname))
        return 0;
    printf("Module %s couldn't been loaded\n", modname);
    return -1;
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: %s <module name>\n", argv[0]);
        return 0;
    }
    else
        return load_module(argv[1]);
}
