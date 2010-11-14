#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <unistd.h>
#include <sys/types.h>

#include <stdint.h>

#define VFS_NAMELEN  32
#define VFS_MNAMELEN 1024

typedef struct fsid
{
    int32_t val[2];
} fsid_t;

struct statvfs
{
    unsigned long f_bsize;
    unsigned long f_frsize;
    fsblkcnt_t f_blocks;
    fsblkcnt_t f_bfree;
    fsblkcnt_t f_bavail;
    fsfilcnt_t f_files;
    fsfilcnt_t f_ffree;
    fsfilcnt_t f_favail;
    unsigned long f_fsid;
    unsigned long f_flag;
    unsigned long f_namemax;
    
    char f_fstypename[VFS_NAMELEN];
    char f_mntonname[VFS_MNAMELEN];
    char f_mntfromname[VFS_MNAMELEN];
};

#define ST_RDONLY    1
#define ST_NOSUID    2

#define MNT_NOWAIT      1
#define MNT_RDONLY      2

#define MS_RDONLY       1
#define MS_NOSUID       2
#define MS_NODEV        4
#define MS_NOEXEC       8
#define MS_SYNCHRONOUS  16
#define MS_REMOUNT      32
#define MS_MANDLOCK     64
#define MS_DIRSYNC      128
#define MS_NOATIME      1024
#define MS_NODIRATIME   2048
#define MS_BIND         4096
#define MS_MOVE         8192
#define MS_REC          16384
#define MS_VERBOSE      32768

#include <sys/statvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(getmntinfo, (struct statvfs **mntbufp, int flags));

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
