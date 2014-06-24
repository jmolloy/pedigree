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


#include "newlib.h"

#include <sys/errno.h>
#include <stdio.h>
#include <string.h>
#include <utmpx.h>

static FILE *utmp = 0;

void endutxent(void)
{
    if (utmp)
    {
        fclose(utmp);
        utmp = 0;
    }
}

struct utmpx *getutxent(void)
{
    static struct utmpx ut;

    if (!utmp)
    {
        utmp = fopen(UTMP_FILE, "w+");
        if (!utmp)
            return 0;
    }

    size_t n = fread(&ut, sizeof(struct utmpx), 1, utmp);
    if (!n)
        return 0; // EOF

    return &ut;
}

struct utmpx *do_getutxid(const struct utmpx *ut, int pid_check)
{
    struct utmpx *p = 0;
    do
    {
        p = getutxent();
        if (p)
        {
            if ((ut->ut_type == BOOT_TIME || ut->ut_type == OLD_TIME ||
                 ut->ut_type == NEW_TIME) && (p->ut_type == ut->ut_type))
            {
                break;
            }
            else if ((p->ut_type == ut->ut_type) && (!strcmp(p->ut_id, ut->ut_id)))
            {
                break;
            }
            else if (p->ut_pid == ut->ut_pid)
                break;
        }
    } while(p);

    return p;
}

struct utmpx *getutxid(const struct utmpx *ut)
{
    if ((!ut) || (ut->ut_type == EMPTY))
    {
        errno = EINVAL;
        return 0;
    }

    if (!utmp)
    {
        utmp = fopen(UTMP_FILE, "w+");
        if (!utmp)
            return 0;
    }

    // Search forwards, save our place.
    off_t at = ftell(utmp);

    struct utmpx *p = do_getutxid(ut, 0);

    fseek(utmp, at, SEEK_SET);
    return p;
}

struct utmpx *getutxline(const struct utmpx *ut)
{
    if (!ut)
    {
        errno = EINVAL;
        return 0;
    }

    if (!utmp)
    {
        utmp = fopen(UTMP_FILE, "w+");
        if (!utmp)
            return 0;
    }

    // Search forwards, save our place.
    off_t at = ftell(utmp);

    struct utmpx *p = 0;
    do
    {
        p = getutxent();
        if (p)
        {
            if ((p->ut_type == LOGIN_PROCESS) || (p->ut_type == USER_PROCESS))
            {
                if (!strcmp(ut->ut_line, p->ut_line))
                    goto done;
            }
        }
    } while(p);

done:
    fseek(utmp, at, SEEK_SET);
    return p;
}

struct utmpx *pututxline(const struct utmpx *ut)
{
    if (!utmp)
    {
        utmp = fopen(UTMP_FILE, "w+");
        if (!utmp)
            return 0;
    }

    // Search forwards, save our place.
    off_t at = ftell(utmp);
    struct utmpx *p = do_getutxid(ut, 1);
    if (p)
    {
        // Found a match, replace it.
        fseek(utmp, -sizeof(struct utmpx), SEEK_CUR);
        fwrite(ut, sizeof(struct utmpx), 1, utmp);
    }
    else
    {
        fseek(utmp, 0, SEEK_END);
        fwrite(ut, sizeof(struct utmpx), 1, utmp);
    }
    fseek(utmp, at, SEEK_SET);

    return getutxid(ut);
}

void setutxent()
{
    if (utmp)
    {
        fseek(utmp, 0, SEEK_SET);
    }
}
