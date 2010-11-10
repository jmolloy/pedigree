#ifndef _MNTENT_H
#define _MNTENT_H

#include <paths.h>

struct mntent
{
    char *mnt_fsname;
    char *mnt_dir;
    char *mnt_type;;
    char *mnt_opts;
}

#endif

