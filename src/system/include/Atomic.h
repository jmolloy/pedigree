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

#ifndef KERNEL_ATOMIC_H
#define KERNEL_ATOMIC_H

#include <compiler.h>

/** @addtogroup kernel
 * @{ */

// NOTE: See http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html
//       for more information about gcc's builtin atomic operations

/** Wrapper around gcc's builtin atomic operations */
template<typename T>
class Atomic
{
  public:
    /** The constructor
     *\param[in] value initial value */
    inline Atomic(T value = T())
      : m_Atom(value){}
    /** The copy-constructor
     *\param[in] x reference object */
    inline Atomic(const Atomic &x)
      : m_Atom(x.m_Atom){}
    /** The assignment operator
     *\param[in] x reference object */
    inline Atomic &operator = (const Atomic &x)
    {
      m_Atom = x.m_Atom;
      return *this;
    }
    /** The destructor does nothing */
    inline ~Atomic(){}

    /** Addition
     *\param[in] x value to add
     *\return the value after the addition */
    inline T operator += (T x)
    {
      return __sync_add_and_fetch(&m_Atom, x);
    }
    /** Subtraction
     *\param[in] x value to subtract
     *\return the value after the subtraction */
    inline T operator -= (T x)
    {
      return __sync_sub_and_fetch(&m_Atom, x);
    }
    /** Bitwise or
     *\param[in] x the operand
     *\return the value after the bitwise or */
    inline T operator |= (T x)
    {
      return __sync_or_and_fetch(&m_Atom, x);
    }
    /** Bitwise and
     *\param[in] x the operand
     *\return the value after the bitwise and */
    inline T operator &= (T x)
    {
      return __sync_and_and_fetch(&m_Atom, x);
    }
    /** Bitwise xor
     *\param[in] x the operand
     *\return the value after the bitwise xor */
    inline T operator ^= (T x)
    {
      return __sync_xor_and_fetch(&m_Atom, x);
    }
    /** Compare and swap
     *\param[in] oldVal the comparision value
     *\param[in] newVal the new value
     *\return true, if the Atomic had the value oldVal and the value was changed to newVal, false otherwise */
    inline bool compareAndSwap(T oldVal, T newVal)
    {
      return __sync_bool_compare_and_swap(&m_Atom, oldVal, newVal);
    }
    /** Get the value
     *\return the value of the Atomic */
    inline operator T () const
    {
      return m_Atom;
    }

  private:
    /** The atomic value */
    volatile T m_Atom;
};

/** @} */

#endif
