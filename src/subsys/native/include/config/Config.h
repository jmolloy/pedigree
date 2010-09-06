/*
 * Copyright (c) 2010 James Molloy, Eduard Burtescu
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

#include <types.h>
#include <string>

using namespace std;

namespace Config
{
    class Result
    {
        public:
            Result(size_t nResultIdx) : m_nResultIdx(nResultIdx) {}
            ~Result();

            /// Returns true if the result is valid, false if there was an error
            bool succeeded();
            /// Returns the error message
            string errorMessage(size_t buffSz = 256);

            /// Returns the number of rows
            size_t rows();
            /// Returns the number of columns
            size_t cols();

            /// Returns the name of the col'th column
            string getColumnName(size_t col, size_t buffSz = 256);

            /// Returns the value in column 'col' of the result, in string form
            string getStr(size_t row, size_t col, size_t buffSz = 256);
            /// Returns the value in column 'col' of the result, in number form
            size_t getNum(size_t row, size_t col);
            /// Returns the value in column 'col' of the result, in boolean form
            bool getBool(size_t row, size_t col);

            /// Returns the value in the column called 'col', in string form
            string getStr(size_t row, const char *col, size_t buffSz = 256);
            /// Returns the value in the column called 'col', in number form
            size_t getNum(size_t row, const char *col);
            /// Returns the value in the column called 'col', in boolean form
            bool getBool(size_t row, const char *col);

        private:
            Result(const Result &);
            Result &operator = (const Result &);

            size_t m_nResultIdx;
    };

    /// Performs a select/update/insert/whatever query on the database
    Result *query(const char *sql);
};

#endif
