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

#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void pedigree_module_unload(char *name);
extern int pedigree_module_is_loaded(char *name);
extern int pedigree_module_get_depending(char *name, char *buf, size_t bufsz);
int unload_module(char *name);

int unload_module(char *name)
{
    if(!pedigree_module_is_loaded(name))
    {
        printf("Module %s isn't loaded!\n", name);
        return -1;
    }
    printf("Module %s being unloaded...\n", name);

    // First unload all dependencies
    char *dep = (char*)malloc(256);
    while(pedigree_module_get_depending(name, dep, 256))
    {
        if(unload_module(dep) == -1)
            return -1;
    }
    free(dep);
    // Finally unload the module
    pedigree_module_unload(name);
    if(!pedigree_module_is_loaded(name))
        return 0;
    printf("Module %s couldn't been unloaded\n", name);
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
        return unload_module(argv[1]);
}
