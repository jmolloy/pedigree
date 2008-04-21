/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef KERNEL_ATOMIC_H
#define KERNEL_ATOMIC_H

#include <compiler.h>

/** @addtogroup kernel
 * @{ */

// NOTE: See http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html
//       for more information about gcc's builtin atomic operations

template<typename T>
class Atomic
{
  public:
    inline Atomic(T value = T())
      : m_Atom(value){}
    inline ~Atomic(){}

    inline T operator += (T x)
    {
      return __sync_add_and_fetch(&m_Atom, x);
    }
    inline T operator -= (T x)
    {
      return __sync_sub_and_fetch(&m_Atom, x);
    }
    inline T operator |= (T x)
    {
      return __sync_or_and_fetch(&m_Atom, x);
    }
    inline T operator &= (T x)
    {
      return __sync_and_and_fetch(&m_Atom, x);
    }
    inline T operator ^= (T x)
    {
      return __sync_xor_and_fetch(&m_Atom, x);
    }
    inline bool compareAndSwap(T oldVal, T newVal)
    {
      return __sync_bool_compare_and_swap(&m_Atom, oldVal, newVal);
    }

  protected:
    inline Atomic(const Atomic &x)
      : m_Atom(x.m_Atom){}
    inline Atomic &operator = (const Atomic &x)
    {
      m_Atom = x.m_Atom;
    }

  private:
    volatile T m_Atom;
};

/** @} */

#endif
