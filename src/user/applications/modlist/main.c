#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_FMT "root»/system/modules/%s.o"

extern int pedigree_module_is_loaded(char *name);

int main(int argc, char **argv)
{
    DIR *dp;
    struct dirent *ep;
    char *moddir = dirname(strdup(MODULE_FMT)), *modsufix = &(basename(strdup(MODULE_FMT))[3]);
    int modsufixlen = strlen(modsufix);
    dp = opendir(moddir);

    if(!dp)
    {
        printf("Couldn't open the directory %s!\n", moddir);
        return -1;
    }
    printf("Modules found in %s:\n", moddir);
    while(ep = readdir(dp))
    {
        if(strlen(ep->d_name)>modsufixlen && !strcmp(&(ep->d_name[strlen(ep->d_name)-modsufixlen]), modsufix))
        {
            ep->d_name[strlen(ep->d_name)-modsufixlen] = 0;
            printf("   %s%s\n", pedigree_module_is_loaded(ep->d_name)?"\e[32m*\e[0m":" ", ep->d_name);
        }
    }
    printf("\e[32m* loaded modules\e[0m\n");
    closedir(dp);
    return 0;
}
