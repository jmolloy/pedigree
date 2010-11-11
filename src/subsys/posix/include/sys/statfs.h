#ifndef _SYS_STATFS_H
#define _SYS_STATFS_H

#ifndef _SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#include <sys/types.h>
#include <sys/mount.h>
#include <unistd.h>

typedef struct fsid
{
    int32_t val[2];
} fsid_t;

struct statfs
{
    int32_t     f_type;
    uint32_t    f_bsize;
    uint32_t    f_blocks;
    uint32_t    f_bfree;
    uint32_t    f_bavail;
    uint32_t    f_files;
    uint32_t    f_ffree;
    fsid_t      f_fsid;
    uint32_t    f_namelen;
    uint32_t    f_spare[6]; // Spare 24 bytes for future expansion without breaking
                            // existing binaries.
};

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(statfs, (const char *path, struct statfs *buf));
int _EXFUN(fstatfs, (int fd, struct statfs *buf));

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
