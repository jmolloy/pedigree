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

#define CHECK_UTMP_FILE(badval) do { \
        if (!utmp) \
        { \
            utmp = fopen(UTMP_FILE, "w"); \
            if (!utmp) \
                return badval; \
        } \
    } while(0)

/**
 * endutxent is a notification from the application that it is done with utmp.
 */
void endutxent(void)
{
    if (utmp)
    {
        fclose(utmp);
        utmp = 0;
    }
}

/**
 * getutxent reads a line from the current file position.
 */
struct utmpx *getutxent(void)
{
    static struct utmpx ut;

    CHECK_UTMP_FILE(0);

    size_t n = fread(&ut, sizeof(struct utmpx), 1, utmp);
    if (!n)
        return 0; // EOF

    return &ut;
}

/**
 * getutxid searches forward to find a match for ut (based on ut_id).
 */
struct utmpx *getutxid(const struct utmpx *ut)
{
    if ((!ut) || (ut->ut_type == EMPTY))
    {
        errno = EINVAL;
        return 0;
    }

    CHECK_UTMP_FILE(0);

    // Search forwards.
    struct utmpx *p = 0;
    do
    {
        p = getutxent();
        if (p)
        {
            if ((ut->ut_type >= RUN_LVL && ut->ut_type <= NEW_TIME) &&
                (p->ut_type == ut->ut_type))
            {
                break;
            }
            else if (!strcmp(p->ut_id, ut->ut_id))
            {
                break;
            }
        }
    } while(p);

    return p;
}

/**
 * getutxline searches forward to find a match for ut (based on ut_line).
 */
struct utmpx *getutxline(const struct utmpx *ut)
{
    if (!ut)
    {
        errno = EINVAL;
        return 0;
    }

    CHECK_UTMP_FILE(0);

    struct utmpx *p = 0;
    do
    {
        p = getutxent();
        if (p)
        {
            if ((p->ut_type == LOGIN_PROCESS) || (p->ut_type == USER_PROCESS))
            {
                if (!strcmp(ut->ut_line, p->ut_line))
                    break;
            }
        }
    } while(p);

    return p;
}

/**
 * pututxline writes ut to the utmp file.
 */
struct utmpx *pututxline(const struct utmpx *ut)
{
    if (!ut)
    {
        errno = EINVAL;
        return 0;
    }

    CHECK_UTMP_FILE(0);

    // Fidn a match for the given entry.
    struct utmpx *p = getutxid(ut);
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

    return getutxid(ut);
}

/**
 * setutxent rewinds to the beginning of the file.
 */
void setutxent()
{
    if (utmp)
    {
        fseek(utmp, 0, SEEK_SET);
    }
}
