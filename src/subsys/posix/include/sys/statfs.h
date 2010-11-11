#ifndef _SYS_STATFS_H
#define _SYS_STATFS_H

#include <sys/mount.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(statfs, (const char *path, struct statfs *buf));
int _EXFUN(fstatfs, (int fd, struct statfs *buf));

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
