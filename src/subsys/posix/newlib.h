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

#ifndef NEWLIB_H
#define NEWLIB_H

#ifdef HOSTED
#ifdef __cplusplus
namespace __pedigree_hosted {};
using namespace __pedigree_hosted;
#endif
#endif

#ifndef _COMPILING_SUBSYS
#define _COMPILING_SUBSYS
#endif

#ifndef _PEDIGREE_COMPILING_SUBSYS
#define _PEDIGREE_COMPILING_SUBSYS
#endif

#ifdef _EXFUN
#undef _EXFUN
#undef _ANSIDECL_H_
#include <_ansi.h>
#endif

#define _POSIX_THREADS
#include <fcntl.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <grp.h>

#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>

#include <stdlib.h>
#include <stdarg.h>
#include <utime.h>

#include <sys/utsname.h>

#include <semaphore.h>
#include <pthread.h>

#define SYS_SOCK_CONSTANTS_ONLY
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <poll.h>

#include <syslog.h>

#ifdef _WANT_STRING_H
#include <string.h>
#endif

#include <sys/mount.h>

#endif
