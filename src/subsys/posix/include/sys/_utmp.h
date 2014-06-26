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

#ifndef _SYS_UTMP_H
#define _SYS_UTMP_H

#include <sys/types.h>
#include <sys/time.h>
#include <paths.h>

#define UTMP_FILE _PATH_UTMP
#define WTMP_FILE "/system/wtmp"
#define PATH_WTMP WTMP_FILE

#define UT_LINESIZE     64
#define UT_NAMESIZE     64
#define UT_HOSTSIZE     256

#define EMPTY           0
#define RUN_LVL         1
#define BOOT_TIME       2
#define OLD_TIME        3
#define NEW_TIME        4
#define USER_PROCESS    5
#define INIT_PROCESS    6
#define LOGIN_PROCESS   7
#define DEAD_PROCESS    8

#endif

#ifdef __UTMP_STRUCT
struct __UTMP_STRUCT {
    char ut_user[UT_NAMESIZE];
    char ut_id[UT_LINESIZE];
    char ut_line[UT_LINESIZE];
    pid_t ut_pid;
    short ut_type;
    struct timeval ut_tv;
};
#endif

#ifndef ut_name
#define ut_name ut_user
#endif
