#ifndef _FSTAB_H
#define _FSTAB_H

struct fstab {
     char *fs_spec;          /* block device name */
     char *fs_file;          /* mount point */
     char *fs_vfstype;       /* filesystem type */
     char *fs_mntops;        /* mount options */
     const char *fs_type;    /* rw/rq/ro/sw/xx option */
     int fs_freq;            /* dump frequency, in days */
     int fs_passno;          /* pass number on parallel dump */
};

struct fstab *_EXFUN(getfsent, (void));
struct fstab *_EXFUN(getfsfile, (const char *mount_point));
struct fstab *_EXFUN(getfsspec, (const char *special_file));
int _EXFUN(setfsent, (void));
void _EXFUN(endfsent, (void));

#endif
