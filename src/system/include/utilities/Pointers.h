/*
 * Copyright (c) 2013 James Molloy, Jörg Pfähler, Matthew Iselin
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
#ifndef POINTERS_H
#define POINTERS_H

#include <processor/types.h>
#include <Log.h>

/** Provides a wrapper around a single-use pointer. The copy constructor
  * will invalidate the reference in the object being copied from.
  */
template <class T>
class UniquePointer
{
    public:
        UniquePointer(T *p) : m_Pointer(p)
        {
        }

        virtual ~UniquePointer()
        {
            if(m_Pointer)
            {
                delete m_Pointer;
                m_Pointer = 0;
            }
        }

        UniquePointer(UniquePointer<T> &p)
        {
            m_Pointer = p.m_Pointer;
            p.m_Pointer = 0;
        }

        UniquePointer<T> &operator = (UniquePointer<T> &p)
        {
            m_Pointer = p.m_Pointer;
            p.m_Pointer = 0;

            return *this;
        }

        T * operator * ()
        {
            return m_Pointer;
        }

    private:

        T *m_Pointer;
};

#endif
