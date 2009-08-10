#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#define MNT_NOWAIT      1
#define MNT_RDONLY      2

int getmntinfo(struct statfs **mntbufp, int flags);

#endif
