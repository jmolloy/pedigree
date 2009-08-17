#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#define MNT_NOWAIT      1
#define MNT_RDONLY      2

#ifdef __cplusplus
extern "C" {
#endif

int getmntinfo(struct statfs **mntbufp, int flags);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
