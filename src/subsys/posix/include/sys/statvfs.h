#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#include <sys/mount.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(statvfs, (const char *, struct statvfs *));
int _EXFUN(fstatvfs, (int, struct statvfs *));

#ifdef __cplusplus
};
#endif

#endif
