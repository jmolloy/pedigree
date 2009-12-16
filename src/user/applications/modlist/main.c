#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/// \todo Put the path to the module directory in the configuration database
#define MODULE_DIR "root»/system/modules"

extern int pedigree_module_is_loaded(char *name);

/// \todo Options to show all modules (default), only loaded modules, and only
///       unloaded modules.
int main(int argc, char **argv)
{
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
            *(suffix - 1) = '\0';
            printf("%-32s%s", filename, pedigree_module_is_loaded(filename) ? "[loaded]" : "");
            printf("\n");
        }
    }
    closedir(dp);
    return 0;
}
