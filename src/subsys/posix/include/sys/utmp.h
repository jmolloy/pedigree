#ifndef _SYS_UTMP_H
#define _SYS_UTMP_H


#include <sys/types.h>


#define UTMP_FILE "/support/utmp"
#define WTMP_FILE "/support/wtmp"
#define PATH_WTMP WTMP_FILE

#define UT_LINESIZE     32
#define UT_NAMESIZE     32
#define UT_HOSTSIZE     256

struct utmp {
    short int ut_type;
    pid_t ut_pid;
    char ut_line[UT_LINESIZE];
    char ut_id[4];
    union
    {
        char ut_user[UT_NAMESIZE];
        char ut_name[UT_NAMESIZE];
    } a;
    char ut_host[UT_HOSTSIZE];
    time_t ut_time;
    char __filler[48];
};

#define ut_user a.ut_user
#define ut_name a.ut_name

#define RUN_LVL         1
#define BOOT_TIME       2
#define NEW_TIME        3
#define OLD_TIME        4

#define INIT_PROCESS    5
#define LOGIN_PROCESS   6
#define USER_PROCESS    7
#define DEAD_PROCESS    8

#endif
