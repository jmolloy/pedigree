#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#include <sys/types.h>
#include <sys/mount.h>
#include <unistd.h>

#define VFS_NAMELEN  32
#define VFS_MNAMELEN 1024

struct statvfs
{
    unsigned long f_bsize;
    unsigned long f_frsize;
    fsblkcnt_t f_blocks;
    fsblkcnt_t f_bfree;
    fsblkcnt_t f_bavail;
    fsblkcnt_t f_files;
    fsblkcnt_t f_ffree;
    fsblkcnt_t f_favail;
    unsigned long f_fsid;
    unsigned long f_flag;
    unsigned long f_namemax;

    char f_fstypename[VFS_NAMELEN];
    char f_mntonname[VFS_MNAMELEN];
    char f_mntfromname[VFS_MNAMELEN];
};

#ifndef _STATFS_T
#define _STATFS_T
typedef struct statvfs struct statfs;
#endif

#define ST_RDONLY    1
#define ST_NOSUID    2

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(statvfs, (const char *, struct statvfs *));
int _EXFUN(fstatvfs, (int, struct statvfs *));

#ifdef __cplusplus
};
#endif

#endif
