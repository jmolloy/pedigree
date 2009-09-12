/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#include "pedigree-syscalls.h"
#include <config/Config.h>
#include <Log.h>

#define MAX_RESULTS 32
static Config::Result *g_Results[MAX_RESULTS];
static size_t g_Rows[MAX_RESULTS];

void pedigree_config_init()
{
    memset(g_Results, 0, sizeof(Config::Result*)*MAX_RESULTS);
    memset(g_Rows, 0, sizeof(size_t)*MAX_RESULTS);
}

void pedigree_config_getcolname(size_t resultIdx, size_t n, char *buf, size_t bufsz)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    String str = g_Results[resultIdx]->getColumnName(n);
    strncpy(buf, static_cast<const char*>(str), bufsz);
}

void pedigree_config_getstr(size_t resultIdx, size_t n, char *buf, size_t bufsz)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    String str = g_Results[resultIdx]->getStr(g_Rows[resultIdx], n);
    strncpy(buf, static_cast<const char*>(str), bufsz);
}

void pedigree_config_getstr(size_t resultIdx, const char *col, char *buf, size_t bufsz)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    String str = g_Results[resultIdx]->getStr(g_Rows[resultIdx], col);
    strncpy(buf, static_cast<const char*>(str), bufsz);
}

int pedigree_config_getnum(size_t resultIdx, size_t n)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getNum(g_Rows[resultIdx], n));
}

int pedigree_config_getnum(size_t resultIdx, const char *col)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getNum(g_Rows[resultIdx], col));
}

int pedigree_config_getbool(size_t resultIdx, size_t n)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getBool(g_Rows[resultIdx], n));
}

int pedigree_config_getbool(size_t resultIdx, const char *col)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return 0;
    return static_cast<int>(g_Results[resultIdx]->getNum(g_Rows[resultIdx], col));
}

int pedigree_config_query(const char *query)
{
    for (size_t i = 0; i < MAX_RESULTS; i++)
    {
        if (g_Results[i] == 0)
        {
            g_Results[i] = Config::instance().query(query);
            // Start at -1, then the first nextrow() will advance to 0.
            g_Rows[i] = -1;
            return i;
        }
    }
    ERROR("Insufficient free resultsets.");
    return -1;
}

void pedigree_config_freeresult(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return;
    delete g_Results[resultIdx];
    g_Rows[resultIdx] = -1;
    g_Results[resultIdx] = 0;
}

int pedigree_config_numcols(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return -1;
    return g_Results[resultIdx]->cols();
}

int pedigree_config_numrows(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return -1;
    return g_Results[resultIdx]->rows();
}

int pedigree_config_nextrow(size_t resultIdx)
{
    if (resultIdx >= MAX_RESULTS || g_Results[resultIdx] == 0)
        return -1;

    if (g_Rows[resultIdx]+1 >= g_Results[resultIdx]->rows())
        return -1;
    else
        g_Rows[resultIdx] ++;
    return 0;
}
