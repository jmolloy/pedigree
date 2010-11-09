#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <unistd.h>

struct statvfs;

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

#ifdef __cplusplus
extern "C" {
#endif

int _EXFUN(getmntinfo, (struct statvfs **mntbufp, int flags));

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
