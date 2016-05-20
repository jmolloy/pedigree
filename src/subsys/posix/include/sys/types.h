/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* unified sys/types.h:
   start with sef's sysvi386 version.
   merge go32 version -- a few ifdefs.
   h8300hms, h8300xray, and sysvnecv70 disagree on the following types:

   typedef int gid_t;
   typedef int uid_t;
   typedef int dev_t;
   typedef int ino_t;
   typedef int mode_t;
   typedef int caddr_t;

   however, these aren't "reasonable" values, the sysvi386 ones make far
   more sense, and should work sufficiently well (in particular, h8300
   doesn't have a stat, and the necv70 doesn't matter.) -- eichin
 */

#ifndef _SYS_TYPES_H

#include <_ansi.h>

#ifndef __INTTYPES_DEFINED__
#define __INTTYPES_DEFINED__

#include <machine/_types.h>

struct sigevent
{
    int donotuse;
};

#if defined(__rtems__)
/*
 *  The following section is RTEMS specific and is needed to more
 *  closely match the types defined in the BSD sys/types.h.
 *  This is needed to let the RTEMS/BSD TCP/IP stack compile.
 */

/* deprecated */
#if ___int8_t_defined
typedef __uint8_t	u_int8_t;
#endif
#if ___int16_t_defined
typedef __uint16_t	u_int16_t;
#endif
#if ___int32_t_defined
typedef __uint32_t	u_int32_t;
#endif

#if ___int64_t_defined
typedef __uint64_t	u_int64_t;

/* deprecated */
typedef	__uint64_t	u_quad_t;
typedef	__int64_t	quad_t;
typedef	quad_t *	qaddr_t;
#endif

#endif

#endif /* ! __INTTYPES_DEFINED */

#ifndef __need_inttypes

#define _SYS_TYPES_H
#include <sys/_types.h>

#ifdef __i386__
#if defined (GO32) || defined (__MSDOS__)
#define __MS_types__
#endif
#endif

# include <stddef.h>
# include <machine/types.h>

# ifndef _CLOCK_T
#   define _CLOCK_T_	unsigned long		/* clock() */
# endif
# ifndef _TIME_T
#   define _TIME_T_ long
# endif
# ifndef _CLOCKID_T_
#   define _CLOCKID_T_ 	unsigned long
# endif
# ifndef _TIMER_T_
#   define _TIMER_T_   	unsigned long
# endif

/* To ensure the stat struct's layout doesn't change when sizeof(int), etc.
   changes, we assume sizeof short and long never change and have all types
   used to define struct stat use them and not int where possible.
   Where not possible, _ST_INTxx are used.  It would be preferable to not have
   such assumptions, but until the extra fluff is necessary, it's avoided.
   No 64 bit targets use stat yet.  What to do about them is postponed
   until necessary.  */
#ifdef __GNUC__
#define _ST_INT32 __attribute__ ((__mode__ (__SI__)))
#else
#define _ST_INT32
#endif

# ifndef	_POSIX_SOURCE

#  define	physadr		physadr_t
#  define	quad		quad_t

#ifndef _BSDTYPES_DEFINED
/* also defined in mingw/gmon.h and in w32api/winsock[2].h */
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
#define _BSDTYPES_DEFINED
#endif

typedef	unsigned short	ushort;		/* System V compatibility */
typedef	unsigned int	uint;		/* System V compatibility */
# endif	/*!_POSIX_SOURCE */

#ifndef __clock_t_defined
typedef _CLOCK_T_ clock_t;
#define __clock_t_defined
#endif

#ifndef __time_t_defined
typedef _TIME_T_ time_t;
#define __time_t_defined

/* Time Value Specification Structures, P1003.1b-1993, p. 261 */

struct timespec {
  time_t  tv_sec;   /* Seconds */
  long    tv_nsec;  /* Nanoseconds */
};

struct itimerspec {
  struct timespec  it_interval;  /* Timer period */
  struct timespec  it_value;     /* Timer expiration */
};
#endif

typedef	long	daddr_t;
typedef	char *	caddr_t;

#ifndef __CYGWIN__
#if defined(__MS_types__) || defined(__rtems__) || \
    defined(__sparc__) || defined(__SPU__)
typedef	unsigned long	ino_t;
#else
typedef	unsigned short	ino_t;
#endif
#endif /*__CYGWIN__*/

#ifdef __MS_types__
typedef unsigned long vm_offset_t;
typedef unsigned long vm_size_t;

#define __BIT_TYPES_DEFINED__

typedef signed char int8_t;
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef int int32_t;
typedef unsigned int u_int32_t;
typedef long long int64_t;
typedef unsigned long long u_int64_t;
typedef int32_t register_t;
#endif /* __MS_types__ */

/*
 * All these should be machine specific - right now they are all broken.
 * However, for all of Cygnus' embedded targets, we want them to all be
 * the same.  Otherwise things like sizeof (struct stat) might depend on
 * how the file was compiled (e.g. -mint16 vs -mint32, etc.).
 */

#if defined(__rtems__)
/* device numbers are 32-bit major and and 32-bit minor */
typedef unsigned long long dev_t;
#else
#ifndef __CYGWIN__
typedef	short	dev_t;

#define makedev(m1, m2)  ((dev_t) (((m1 & 0xFF) << 8) | (m2 & 0xFF)))
#define major(d)         ((d >> 8) & 0xFF)
#define minor(d)         (d & 0xFF)
#endif
#endif

#ifndef __CYGWIN__	/* which defines these types in it's own types.h. */
typedef long		off_t;

typedef	unsigned int	uid_t;
typedef	unsigned int	gid_t;
#endif

typedef int pid_t;
#ifndef __CYGWIN__
typedef	long key_t;
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef _ssize_t ssize_t;
#endif

#ifndef __CYGWIN__
#ifdef __MS_types__
typedef	char *	addr_t;
typedef int mode_t;
#else
#if defined (__sparc__) && !defined (__sparc_v9__)
#ifdef __svr4__
typedef unsigned long mode_t;
#else
typedef unsigned short mode_t;
#endif
#else
typedef unsigned int mode_t _ST_INT32;
#endif
#endif /* ! __MS_types__ */
#endif /*__CYGWIN__*/

typedef unsigned short nlink_t;

#undef __MS_types__
#undef _ST_INT32


#ifndef __clockid_t_defined
typedef _CLOCKID_T_ clockid_t;
#define __clockid_t_defined
#endif

#ifndef __timer_t_defined
typedef _TIMER_T_ timer_t;
#define __timer_t_defined
#endif

typedef unsigned long useconds_t;
typedef long suseconds_t;

#include <sys/features.h>


/* Cygwin will probably never have full posix compliance due to little things
 * like an inability to set the stackaddress. Cygwin is also using void *
 * pointers rather than structs to ensure maximum binary compatability with
 * previous releases.
 * This means that we don't use the types defined here, but rather in
 * <cygwin/types.h>
 */
#if 0 &&  defined(_POSIX_THREADS) && !defined(__CYGWIN__)

#include <sys/sched.h>

/*
 *  2.5 Primitive System Data Types,  P1003.1c/D10, p. 19.
 */

typedef __uint32_t pthread_t;            /* identify a thread */

/* P1003.1c/D10, p. 118-119 */
#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM  1

/* P1003.1c/D10, p. 111 */
#define PTHREAD_INHERIT_SCHED  1      /* scheduling policy and associated */
                                      /*   attributes are inherited from */
                                      /*   the calling thread. */
#define PTHREAD_EXPLICIT_SCHED 2      /* set from provided attribute object */

/* P1003.1c/D10, p. 141 */
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE  1

typedef struct {
  int is_initialized;
  void *stackaddr;
  int stacksize;
  int contentionscope;
  int inheritsched;
  int schedpolicy;
  struct sched_param schedparam;

  /* P1003.4b/D8, p. 54 adds cputime_clock_allowed attribute.  */
#if defined(_POSIX_THREAD_CPUTIME)
  int  cputime_clock_allowed;  /* see time.h */
#endif
  int  detachstate;

} pthread_attr_t;

#if defined(_POSIX_THREAD_PROCESS_SHARED)
/* NOTE: P1003.1c/D10, p. 81 defines following values for process_shared.  */

#define PTHREAD_PROCESS_PRIVATE 0 /* visible within only the creating process */
#define PTHREAD_PROCESS_SHARED  1 /* visible too all processes with access to */
                                  /*   the memory where the resource is */
                                  /*   located */
#endif

#if defined(_POSIX_THREAD_PRIO_PROTECT)
/* Mutexes */

/* Values for blocking protocol. */

#define PTHREAD_PRIO_NONE    0
#define PTHREAD_PRIO_INHERIT 1
#define PTHREAD_PRIO_PROTECT 2
#endif

typedef __uint32_t pthread_mutex_t;      /* identify a mutex */

typedef struct {
  int   is_initialized;
#if defined(_POSIX_THREAD_PROCESS_SHARED)
  int   process_shared;  /* allow mutex to be shared amongst processes */
#endif
#if defined(_POSIX_THREAD_PRIO_PROTECT)
  int   prio_ceiling;
  int   protocol;
#endif
  int   recursive;
} pthread_mutexattr_t;

/* Condition Variables */

typedef __uint32_t pthread_cond_t;       /* identify a condition variable */

typedef struct {
  int   is_initialized;
#if defined(_POSIX_THREAD_PROCESS_SHARED)
  int   process_shared;       /* allow this to be shared amongst processes */
#endif
} pthread_condattr_t;         /* a condition attribute object */

/* Keys */

typedef __uint32_t pthread_key_t;        /* thread-specific data keys */

typedef struct {
  int   is_initialized;  /* is this structure initialized? */
  int   init_executed;   /* has the initialization routine been run? */
} pthread_once_t;       /* dynamic package initialization */
#else
#if defined (__CYGWIN__)
#include <cygwin/types.h>
#endif
#endif /* defined(_POSIX_THREADS) */

/* POSIX Barrier Types */

#if defined(_POSIX_BARRIERS)
typedef __uint32_t pthread_barrier_t;        /* POSIX Barrier Object */
typedef struct {
  int   is_initialized;  /* is this structure initialized? */
#if defined(_POSIX_THREAD_PROCESS_SHARED)
  int   process_shared;       /* allow this to be shared amongst processes */
#endif
} pthread_barrierattr_t;
#endif /* defined(_POSIX_BARRIERS) */

/* POSIX Spin Lock Types */

#if defined(_POSIX_SPIN_LOCKS)
typedef __uint32_t pthread_spinlock_t;        /* POSIX Spin Lock Object */
#endif /* defined(_POSIX_SPIN_LOCKS) */

/* POSIX Reader/Writer Lock Types */

#if defined(_POSIX_READER_WRITER_LOCKS)
typedef __uint32_t pthread_rwlock_t;         /* POSIX RWLock Object */
typedef struct {
  int   is_initialized;       /* is this structure initialized? */
#if defined(_POSIX_THREAD_PROCESS_SHARED)
  int   process_shared;       /* allow this to be shared amongst processes */
#endif
} pthread_rwlockattr_t;
#endif /* defined(_POSIX_READER_WRITER_LOCKS) */

#ifndef _POSIX_THREAD_TYPES_DEFINED
#define _POSIX_THREAD_TYPES_DEFINED

#define _PTHREAD_TYPE_MINSIZE    128

typedef struct _pthread_attr_t
{
    union
    {
        struct
        {
            size_t stackSize;
            int detachState;
            __uint32_t magic; // == _PTHREAD_ATTR_MAGIC when initialised
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_attr_t;

typedef struct _pthread_condattr_t
{
    union
    {
        struct
        {
            clockid_t clock_id;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_condattr_t;

typedef struct _pthread_mutexattr_t
{
    union
    {
        struct
        {
            int type; // PTHREAD_MUTEX_NORMAL,PTHREAD_MUTEX_RECURSIVE, etc
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_mutexattr_t;

typedef struct _pthread_rwlockattr_t
{
    union
    {
        struct
        {
            char _ign;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_rwlockattr_t;

typedef struct _pthread_key_t
{
    union
    {
        struct
        {
            size_t key;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_key_t;

typedef struct _pthread_once_t
{
    union
    {
        struct
        {
            int control;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_once_t;

typedef struct _pthread_t
{
    union
    {
        struct
        {
            int kthread;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} *pthread_t;

typedef struct _pthread_spinlock_t
{
    union
    {
        struct
        {
            char atom;
            pthread_t owner;
            pthread_t locker;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_spinlock_t;

typedef struct _pthread_mutex_t
{
    union
    {
        struct
        {
            __int32_t value;

            // Stored attributes of the mutex.
            pthread_t owner;
            pthread_mutexattr_t attr;

            // Synchronisation components.
            void *waiter;
        } __internal;

        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_mutex_t;

#define pthread_cond_t pthread_mutex_t

typedef struct _pthread_rwlock_t
{
    union
    {
        pthread_mutex_t mutex;
        char __pad[_PTHREAD_TYPE_MINSIZE];
    };
} pthread_rwlock_t;

#endif

typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;

#endif  /* !__need_inttypes */

#undef __need_inttypes

#endif	/* _SYS_TYPES_H */
