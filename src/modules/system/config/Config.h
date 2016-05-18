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

#ifndef CONFIG_H
#define CONFIG_H

#include <processor/types.h>
#include <utilities/String.h>
#include "sqlite3/sqlite3.h"

extern sqlite3 *g_pSqlite;

/** The configuration system for Pedigree.
 *
 * The system is database-based (currently using SQLite).
 */
class Config
{
public:
    class Result
    {
    public:
        Result(char **ppResult, size_t rows, size_t cols, char *error, int ret);
        ~Result();

        /** Returns true if the result is valid, false if there was an error. */
        bool succeeded()
        {return m_Ret == 0;}
        /** Returns the error message. */
        char *errorMessage()
        {return m_pError;}

        /** Returns the number of rows. */
        size_t rows()
        {return m_Rows;}
        /** Returns the number of columns. */
        size_t cols()
        {return m_Cols;}

        /** Returns the name of the n'th column. */
        String getColumnName(size_t n);

        /** Returns the value in column 'n' of the result, in String form. */
        String getStr(size_t row, size_t n);
        /** Returns the value in column 'n' of the result, in number form. */
        size_t getNum(size_t row, size_t n);
        /** Returns the value in column 'n' of the result, in boolean form. */
        bool getBool(size_t row, size_t n);

        /** Returns the value in the column called 'str', in String form. */
        String getStr(size_t row, const char *str);
        /** Returns the value in the column called 'str', in number form. */
        size_t getNum(size_t row, const char *str);
        /** Returns the value in the column called 'str', in boolean form. */
        bool getBool(size_t row, const char *str);

    private:
        Result(const Result &);
        Result &operator = (const Result &);

        size_t lookupCol(const char *str);

        char **m_ppResult;
        size_t m_Rows, m_Cols;
        char  *m_pError;
        int    m_Ret;
    };

    Config();
    ~Config();

    static Config &instance()
    {return m_Instance;}

    /** Performs a select/update/insert/whatever query on the database.
        \return A Result* object, which should be deleted after use, or 0. */
    Result *query(const char *sql);

private:
    static Config m_Instance;
};

#endif
