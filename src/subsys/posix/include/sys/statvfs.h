#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#include <sys/types.h>
#include <unistd.h>

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
};

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
