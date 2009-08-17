#ifndef _SYS_STATFS
#define _SYS_STATFS

struct statfs {
   long    f_type;
   long    f_bsize;    /* optimal transfer block size */
   long    f_blocks;   /* total data blocks in file system */
   long    f_bfree;    /* free blocks in fs */
   long    f_bavail;   /* free blocks avail to non-superuser */
   long    f_files;    /* total file nodes in file system */
   long    f_ffree;    /* free file nodes in fs */
   long    f_fsid;     /* file system id */
   long    f_namelen;  /* maximum length of filenames */
   long    f_flags;    /* mount flags */
   char    f_mntonname[128];    /* mount path */
   char    f_mntfromname[128];  /* device path */
   char    f_fstypename[32];    /* FS name */
};

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
