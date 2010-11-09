#ifndef _SYS_STATFS
#define _SYS_STATFS

#include <sys/statvfs.h>

// Same structure, different name, so much for standards.
#define statfs statvfs

#include <sys/mount.h>

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(statfs, (const char *path, struct statfs *buf));
int _EXFUN(fstatfs, (int fd, struct statfs *buf));

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
