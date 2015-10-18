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

#ifndef POINTERGUARD_H
#define POINTERGUARD_H

#include <processor/types.h>
#include <Log.h>

/** Provides a guard for a pointer. When the class goes out of scope
  * the pointer will automatically be freed.
  */
template <class T>
class PointerGuard
{
  public:
    PointerGuard(T *p = 0) : m_Pointer(p), m_Wrapper(0)
    {
      // NOTICE("PointerGuard: Guarding pointer [" << reinterpret_cast<uintptr_t>(m_Pointer) << "]");
    }

    PointerGuard(T **p = 0) : m_Pointer(*p), m_Wrapper(p)
    {
      // NOTICE("PointerGuard: Guarding pointer [" << reinterpret_cast<uintptr_t>(m_Pointer) << "]");
    }

    virtual ~PointerGuard()
    {
      // NOTICE("PointerGuard: Out-of-scope, deleting guarded pointer [" << reinterpret_cast<uintptr_t>(m_Pointer) << "]");
      if(m_Pointer)
      {
        delete m_Pointer;
        m_Pointer = 0;
      }

      if(m_Wrapper)
      {
        *m_Wrapper = 0;
      }
    }

    /** Neither of these should be used, as they defeat the purpose (and will cause a dual
      * delete of the pointer)
      */

    PointerGuard(PointerGuard<T>& p)
    {
      ERROR("PointerGuard: copy constructor called");
      m_Pointer = 0;
      m_Wrapper = 0;
    }

    PointerGuard<T>& operator = (PointerGuard<T>& p)
    {
      ERROR("PointerGuard: operator = called");
      m_Pointer = 0;
      m_Wrapper = 0;
    }

  private:
    T *m_Pointer;
    T **m_Wrapper;
};

#endif
