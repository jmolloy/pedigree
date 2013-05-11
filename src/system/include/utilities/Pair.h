/*
 * Copyright (c) 2013 Matthew Iselin
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

#ifndef KERNEL_UTILITIES_PAIR_H
#define KERNEL_UTILITIES_PAIR_H

#include <utilities/template.h>
#include <processor/types.h>

/** @addtogroup kernelutilities
 * @{ */

template<class T1, class T2>
class Pair
{
    public:
        Pair() :
            m_First(), m_Second()
        {}
        Pair(T1 &f, T2 &s) :
            m_First(f), m_Second(s)
        {}
        virtual ~Pair()
        {}

        T1 first() const
        {
            return m_First;
        }

        T2 second() const
        {
            return m_Second;
        }

    private:
        T1 m_First;
        T2 m_Second;
};

template<class T1, class T2>
bool operator == (const Pair<T1, T2>& left, const Pair<T1, T2>& right)
{
    return (left.first() == right.first()) && (left.second() == right.second());
}

template<class T1, class T2>
bool operator != (const Pair<T1, T2>& left, const Pair<T1, T2>& right)
{
    return (left.first() != right.first()) || (left.second() != right.second());
}

template<class T1, class T2>
bool operator < (const Pair<T1, T2>& left, const Pair<T1, T2>& right)
{
    if(left.first() < right.first())
    {
        return true;
    }
    else if(right.first() < left.first())
    {
        return false;
    }

    return left.second() < right.second();
}

template<class T1, class T2>
bool operator <= (const Pair<T1, T2>& left, const Pair<T1, T2>& right)
{
    return !(right < left);
}

template<class T1, class T2>
bool operator > (const Pair<T1, T2>& left, const Pair<T1, T2>& right)
{
    return right < left;
}

template<class T1, class T2>
bool operator >= (const Pair<T1, T2>& left, const Pair<T1, T2>& right)
{
    return !(left < right);
}

template<typename T1, typename T2>
Pair<T1,T2> makePair(T1 a, T2 b)
{
    return Pair<T1, T2>(a, b);
}

/** @} */

#endif
