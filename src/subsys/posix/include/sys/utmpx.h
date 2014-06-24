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

#ifndef _SYS_UTMPX_H
#define _SYS_UTMPX_H

#include <sys/types.h>
#include <sys/time.h>

#define UTMP_FILE "/var/run/utmp"
#define WTMP_FILE "/var/run/wtmp"
#define PATH_WTMP WTMP_FILE

#define UT_LINESIZE     64
#define UT_NAMESIZE     64
#define UT_HOSTSIZE     256

struct utmpx {
    char ut_user[UT_NAMESIZE];
    char ut_id[UT_LINESIZE];
    char ut_line[UT_LINESIZE];
    pid_t ut_pid;
    short ut_type;
    struct timeval ut_tv;
};

#define ut_name ut_user

#define EMPTY           0
#define BOOT_TIME       1
#define OLD_TIME        2
#define NEW_TIME        3
#define USER_PROCESS    4
#define INIT_PROCESS    5
#define LOGIN_PROCESS   6
#define DEAD_PROCESS    7

extern void          endutxent(void);
extern struct utmpx *getutxent(void);
extern struct utmpx *getutxid(const struct utmpx *);
extern struct utmpx *getutxline(const struct utmpx *);
extern struct utmpx *pututxline(const struct utmpx *);
extern void          setutxent(void);

#endif
