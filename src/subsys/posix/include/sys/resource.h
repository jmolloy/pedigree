#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

#include <sys/time.h>

typedef unsigned int rlim_t;

#define PRIO_PROCESS        0
#define PRIO_PGRP           1
#define PRIO_USER           2

#define	RUSAGE_SELF         0		/* calling process */
#define	RUSAGE_CHILDREN     -1		/* terminated child processes */

#define RLIM_INFINITY       (-1)
#define RLIM_SAVED_MAX      ((rlim_t) 0)
#define RLIM_SAVED_CUR      ((rlim_t) 1)

#define RLIMIT_CORE         16384
#define RLIMIT_CPU          RLIM_INFINITY
#define RLIMIT_DATA         RLIM_INFINITY
#define RLIMIT_FSIZE        RLIM_INFINITY
#define RLIMIT_NOFILE       RLIM_INFINITY
#define RLIMIT_STACK        RLIM_INFINITY
#define RLIMIT_AS           RLIM_INFINITY

struct rusage {
  	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
};

struct rlimit {
    rlim_t rlim_curr;
    rlim_t rlim_max;
};

int _EXFUN(getrlimit, (int resource, struct rlimit *rlp));
int _EXFUN(setrlimit, (int resource, const struct rlimit *rlp));

#endif

