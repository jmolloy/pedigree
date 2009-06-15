#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

struct utsname
{
    char sysname[128];
    char nodename[128];
    char release[32];
    char version[32];
    char machine[32];
};

int uname(struct utsname *n);

#endif
