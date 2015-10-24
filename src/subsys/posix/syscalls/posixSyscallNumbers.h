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

#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

#define POSIX_OPEN        1
#define POSIX_WRITE       2
#define POSIX_READ        3
#define POSIX_CLOSE       4
#define POSIX_SBRK        5
#define POSIX_FORK        6
#define POSIX_EXECVE      7
#define POSIX_WAITPID     8
#define POSIX_EXIT        9
#define POSIX_OPENDIR    10
#define POSIX_READDIR    11
#define POSIX_REWINDDIR  12
#define POSIX_CLOSEDIR   13
#define POSIX_TCGETATTR  14
#define POSIX_TCSETATTR  15
#define POSIX_IOCTL      16
#define POSIX_STAT       17
#define POSIX_FSTAT      18
#define POSIX_GETPID     19
#define POSIX_CHDIR      20
#define POSIX_SELECT     21
#define POSIX_LSEEK      22

#define POSIX_SOCKET     23
#define POSIX_CONNECT    24
#define POSIX_SEND       25
#define POSIX_RECV       26
#define POSIX_BIND       27
#define POSIX_LISTEN     28
#define POSIX_ACCEPT     29
#define POSIX_RECVFROM   30
#define POSIX_SENDTO     31

#define POSIX_GETTIMEOFDAY 32
#define POSIX_DUP        33
#define POSIX_DUP2       34
#define POSIX_LSTAT      35
#define POSIX_UNLINK     36
#define POSIX_SYMLINK    1000

#define POSIX_GETHOSTBYNAME   37
#define POSIX_GETHOSTBYADDR   38

#define POSIX_FCNTL      39
#define POSIX_PIPE       40

#define POSIX_MKDIR      41
#define POSIX_GETPWENT   42
#define POSIX_GETPWNAM   43
#define POSIX_GETUID     44
#define POSIX_GETGID     45

#define POSIX_SIGACTION  46
#define POSIX_SIGNAL     47
#define POSIX_RAISE      48
#define POSIX_KILL       49
#define POSIX_SIGPROCMASK     50

#define POSIX_ALARM      51
#define POSIX_SLEEP      52

#define POSIX_POLL       56

#define POSIX_RENAME     57

#define POSIX_GETCWD     58

#define POSIX_READLINK   59
#define POSIX_LINK       60

#define POSIX_ISATTY     61

#define POSIX_MMAP       62
#define POSIX_MUNMAP     63

#define POSIX_SHUTDOWN   64

#define POSIX_ACCESS     65

#define POSIX_SETSID     66
#define POSIX_SETPGID    67
#define POSIX_GETPGRP    68

#define POSIX_SIGALTSTACK 69

#define POSIX_SEM_CLOSE     70
#define POSIX_SEM_DESTROY   71
#define POSIX_SEM_GETVALUE  72
#define POSIX_SEM_INIT      73
#define POSIX_SEM_OPEN      74
#define POSIX_SEM_POST      75
#define POSIX_SEM_TIMEWAIT  76
#define POSIX_SEM_TRYWAIT   77
#define POSIX_SEM_UNLINK    78
#define POSIX_SEM_WAIT      79

#define POSIX_PTHREAD_RETURN    80
#define POSIX_PTHREAD_ENTER     81
#define POSIX_PTHREAD_CREATE    82
#define POSIX_PTHREAD_JOIN      83
#define POSIX_PTHREAD_DETACH    84
#define POSIX_PTHREAD_SELF      85
#define POSIX_PTHREAD_KILL      86
#define POSIX_PTHREAD_SIGMASK   87

#define POSIX_PTHREAD_MUTEX_INIT    88
#define POSIX_PTHREAD_MUTEX_DESTROY 89
#define POSIX_PTHREAD_MUTEX_LOCK    90
#define POSIX_PTHREAD_MUTEX_TRYLOCK 91
#define POSIX_PTHREAD_MUTEX_UNLOCK  92

#define POSIX_PTHREAD_KEY_CREATE    93
#define POSIX_PTHREAD_KEY_DELETE    94
#define POSIX_PTHREAD_SETSPECIFIC   95
#define POSIX_PTHREAD_GETSPECIFIC   96
#define POSIX_PTHREAD_KEY_DESTRUCTOR    97

#define POSIX_SYSLOG            98

#define POSIX_FTRUNCATE         99

#define POSIX_STUBBED           100

#define PEDIGREE_SIGRET         101
#define PEDIGREE_INIT_SIGRET    102

#define POSIX_SCHED_YIELD       103

#define POSIX_PEDIGREE_THRWAKEUP  104
#define POSIX_PEDIGREE_THRSLEEP   105

#define POSIX_NANOSLEEP         106
#define POSIX_CLOCK_GETTIME     107

#define POSIX_GETEUID           108
#define POSIX_GETEGID           109
#define POSIX_SETEUID           110
#define POSIX_SETEGID           111

#define POSIX_SETUID            112
#define POSIX_SETGID            113

#define POSIX_CHOWN             114
#define POSIX_CHMOD             115
#define POSIX_FCHOWN            116
#define POSIX_FCHMOD            117
#define POSIX_FCHDIR            118

#define POSIX_STATVFS           119
#define POSIX_FSTATVFS          120

#define PEDIGREE_UNWIND_SIGNAL  121

#define POSIX_MSYNC             122

#define POSIX_GETPEERNAME       123

#define POSIX_FSYNC             124

#define POSIX_MPROTECT          125

#define POSIX_REALPATH          126

#define POSIX_TIMES             127
#define POSIX_GETRUSAGE         128

#define POSIX_PTSNAME           200
#define POSIX_TTYNAME           201
#define POSIX_TCSETPGRP         202
#define POSIX_TCGETPGRP         203

#define POSIX_USLEEP            204

#define POSIX_PEDIGREE_CREATE_WAITER    205
#define POSIX_PEDIGREE_DESTROY_WAITER   206
#define POSIX_PEDIGREE_THREAD_WAIT_FOR  207
#define POSIX_PEDIGREE_THREAD_TRIGGER   209

#define POSIX_PEDIGREE_GET_INFO_BLOCK   210

#endif
