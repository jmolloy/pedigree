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

extern sqlite3 *g_pSqlite;

Config Config::m_Instance;

extern "C" void free(void*);

Config::Result::Result(char **ppResult, size_t rows, size_t cols, char *error, int ret) :
    m_ppResult(ppResult), m_Rows(rows), m_Cols(cols), m_pError(error), m_Ret(ret)
{
}

Config::Result::~Result()
{
    sqlite3_free_table(m_ppResult);
    free(m_pError);
}

String Config::Result::getColumnName(size_t n)
{
    NOTICE("getColname");
    return String(m_ppResult[n]);
}

String Config::Result::getStr(size_t row, size_t n)
{
    NOTICE("getstr(n)");
    return String(m_ppResult[(row+1) * m_Cols + n]);
}

size_t Config::Result::getNum(size_t row, size_t n)
{
    return strtoul(m_ppResult[(row+1) * m_Cols + n], 0, 10);
}

bool Config::Result::getBool(size_t row, size_t n)
{
    const char *s = m_ppResult[(row+1) * m_Cols + n];
    if (!strcmp(s, "true") || !strcmp(s, "True") || !strcmp(s, "1"))
        return true;
    else
        return false;
}

String Config::Result::getStr(size_t row, const char *str)
{
    size_t n = lookupCol(str);
    if (n == ~0UL) return String("");
    return String(m_ppResult[(row+1) * m_Cols + n]);
}

size_t Config::Result::getNum(size_t row, const char *str)
{
    size_t n = lookupCol(str);
    if (n == ~0UL) return 0;
    return strtoul(m_ppResult[(row+1) * m_Cols + n], 0, 10);
}

bool Config::Result::getBool(size_t row, const char *str)
{
    size_t n = lookupCol(str);
    if (n == ~0UL) return false;
    const char *s = m_ppResult[(row+1) * m_Cols + n];
    if (!strcmp(s, "true") || !strcmp(s, "True") || !strcmp(s, "1"))
        return true;
    else
        return false;
}

size_t Config::Result::lookupCol(const char *str)
{
    NOTICE("lookupCol");
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
    NOTICE("config query start");
    char **result;
    int rows, cols;
    char *error;
    int ret = sqlite3_get_table(g_pSqlite, sql, &result, &rows, &cols, &error);
    Result *pR = new Result(result, static_cast<size_t>(rows), static_cast<size_t>(cols), error, ret);
    NOTICE("config query end");
    return pR;
}
