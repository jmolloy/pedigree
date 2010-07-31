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

#include "Config.h"
#include <utilities/utility.h>
#include <Log.h>
#include <process/Mutex.h>
#include <LockGuard.h>

extern sqlite3 *g_pSqlite;
    
Mutex g_sqlLock(false);

Config Config::m_Instance;

extern "C" void free(void*);

Config::Result::Result(char **ppResult, size_t rows, size_t cols, char *error, int ret) :
    m_ppResult(ppResult), m_Rows(rows), m_Cols(cols), m_pError(error), m_Ret(ret)
{
}

Config::Result::~Result()
{
    LockGuard<Mutex> guard(g_sqlLock);
    
    if(m_ppResult)
        sqlite3_free_table(m_ppResult);
    
    free(m_pError);
}

String Config::Result::getColumnName(size_t n)
{
    char *str = m_ppResult[n];
    if (str)
        return String(str);
    return String("");
}

String Config::Result::getStr(size_t row, size_t n)
{
    char *str = m_ppResult[(row+1) * m_Cols + n];
    if (str)
        return String(str);
    return String("");
}

size_t Config::Result::getNum(size_t row, size_t n)
{
    if (m_ppResult[(row+1) * m_Cols + n] == 0)
        return 0;
    return strtoul(m_ppResult[(row+1) * m_Cols + n], 0, 10);
}

bool Config::Result::getBool(size_t row, size_t n)
{
    const char *s = m_ppResult[(row+1) * m_Cols + n];
    if (s == 0)
        return false;
    if (!strcmp(s, "true") || !strcmp(s, "True") || !strcmp(s, "1"))
        return true;
    else
        return false;
}

String Config::Result::getStr(size_t row, const char *str)
{
    size_t n = lookupCol(str);
    if (n == ~0UL) return String("");
    char *str2 = m_ppResult[(row+1) * m_Cols + n];
    if (str2)
        return String(str2);
    return String("");
}

size_t Config::Result::getNum(size_t row, const char *str)
{
    size_t n = lookupCol(str);
    if (n == ~0UL) return 0;
    if (m_ppResult[(row+1) * m_Cols + n] == 0)
        return 0;
    return strtoul(m_ppResult[(row+1) * m_Cols + n], 0, 10);
}

bool Config::Result::getBool(size_t row, const char *str)
{
    size_t n = lookupCol(str);
    if (n == ~0UL) return false;
    const char *s = m_ppResult[(row+1) * m_Cols + n];
    if (s == 0)
        return false;
    if (!strcmp(s, "true") || !strcmp(s, "True") || !strcmp(s, "1"))
        return true;
    else
        return false;
}

size_t Config::Result::lookupCol(const char *str)
{
    for (size_t i = 0; i < m_Cols; i++)
    {
        if (!strcmp(str, m_ppResult[i]))
            return i;
    }
    return ~0UL;
}

Config::Config()
{
}

Config::~Config()
{
}

Config::Result *Config::query(const char *sql)
{
    if(!*sql)
    {
        ERROR("Dud query string passed to Config::query");
        return 0;
    }
    
    LockGuard<Mutex> guard(g_sqlLock);

    char **result;
    int rows, cols;
    char *error;
    int ret = sqlite3_get_table(g_pSqlite, sql, &result, &rows, &cols, &error);
    return new Result(result, static_cast<size_t>(rows), static_cast<size_t>(cols), error, ret);
}

