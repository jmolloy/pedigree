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

/*
So we can get data to the kernel, but we can't really get it back.
One solution might be for the event handler to serialise the buffer,
transmit it to the kernel, and then unserialise the *same buffer*.

This means that the kernel can change stuff inline and it'll affect
the state of both sides of the great divide.

I'll get it working using a system call + serialised buffer, then I'll
investigate other methods of integrating into the event system.
*/

#ifndef _PEDIGREE_TEST_H
#define _PEDIGREE_TEST_H

#include <Object.h>

class Test : public Object
{
    public:
        Test()
        {};
        virtual ~Test()
        {};

        void writeLog(const char *text, size_t len);

        void serialise(uintptr_t *buffer, size_t &length);

        void unserialise(uintptr_t buffer, size_t &length);

    private:
};

#endif
