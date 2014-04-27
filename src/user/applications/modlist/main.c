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

#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// We explicitly define the location of the modules directory to make cross-system
// work easier - keeps consistency across all Pedigree systems.
#define MODULE_DIR "root»/system/modules"

extern int pedigree_module_is_loaded(char *name);

int main(int argc, char **argv)
{
    int listAll = 0, listLoaded = 0, listUnloaded = 0, c = 0, err = 0;
    while((c = getopt(argc, argv, "aluh")) != -1)
    {
        switch(c)
        {
            case 'a':
                if(listLoaded || listUnloaded)
                    err = 1;
                else
                    listAll = 1;
                break;
            case 'l':
                if(listAll || listUnloaded)
                    err = 1;
                else
                    listLoaded = 1;
                break;
            case 'u':
                if(listAll || listLoaded)
                    err = 1;
                else
                    listUnloaded = 1;
                break;
            case 'h':
                fprintf(stderr, "usage: modlist [-a] [-l] [-u]\n");
                fprintf(stderr, "-a%-16slist all modules and show their status\n", "");
                fprintf(stderr, "-l%-16slist all already-loaded modules\n", "");
                fprintf(stderr, "-u%-16slist all modules that are not loaded\n", "");
                fprintf(stderr, "-h%-16sshow this usage information\n", "");
                fprintf(stderr, "\n");
                fprintf(stderr, "modlist without arguments defaults to listing all modules with their status\n");
                err = 3;
                break;
            default:
                fprintf(stderr, "Unrecognized option: -%c\n", optopt);
                err = 2;
        }
    }

    if(!listAll && !listLoaded && !listUnloaded)
        listAll = 1;

    if(err)
    {
        if(err == 1)
            fprintf(stderr, "Only one action (-a, -l, or -u) can be specified\n");
        else if(err == 3)
            return 0;
        return err;
    }

    DIR *dp = opendir(MODULE_DIR);
    if(!dp)
    {
        printf("Couldn't open the directory %s: %s.\n", MODULE_DIR, strerror(errno));
        return -1;
    }

    struct dirent *ep;
    while((ep = readdir(dp)))
    {
        char *filename = ep->d_name;

        char *lastPeriod = strrchr(filename, '.');
        char *suffix = lastPeriod + 1;
        if(lastPeriod && !stricmp(suffix, "o"))
        {
            *lastPeriod = '\0';
            if(listAll)
            {
                printf("%-32s%s\n", filename, pedigree_module_is_loaded(filename) ? "[loaded]" : "");
            }
            else if(listLoaded)
            {
                if(pedigree_module_is_loaded(filename))
                {
                    printf("%s\n", filename);
                }
            }
            else if(listUnloaded)
            {
                if(!pedigree_module_is_loaded(filename))
                {
                    printf("%s\n", filename);
                }
            }
        }
    }
    closedir(dp);
    return 0;
}
