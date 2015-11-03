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

#include <utilities/Vector.h>
#include <utilities/utility.h>

//
// Vector<void*> implementation
//

Vector<void*>::Vector()
 : m_Size(0), m_Count(0), m_Data(0)
{
}
Vector<void*>::Vector(size_t size)
 : m_Size(0), m_Count(0), m_Data(0)
{
  reserve(size, false);
}
Vector<void*>::Vector(const Vector &x)
 : m_Size(0), m_Count(0), m_Data(0)
{
  assign(x);
}
Vector<void*>::~Vector()
{
  if (m_Data != 0)
    delete []m_Data;
}

Vector<void*> &Vector<void*>::operator = (const Vector &x)
{
  assign(x);
  return *this;
}
void *Vector<void*>::operator [](size_t index) const
{
  return m_Data[index];
}

size_t Vector<void*>::size() const
{
  return m_Size;
}
size_t Vector<void*>::count() const
{
  return m_Count;
}
void Vector<void*>::pushBack(void *value)
{
  reserve(m_Count + 1, true);

  m_Data[m_Count++] = value;
}
void *Vector<void*>::popBack()
{
  m_Count--;
  return m_Data[m_Count];
}
void Vector<void*>::pushFront(void *value)
{
  reserve(m_Count + 1, true);
  memmove(&m_Data[1], &m_Data[0], m_Count * sizeof(void*));

  m_Data[0] = value;
  m_Count++;
}
void *Vector<void*>::popFront()
{
  void *ret = m_Data[0];
  m_Count--;
  memmove(&m_Data[0], &m_Data[1], m_Count * sizeof(void*));
  return ret;
}
void Vector<void*>::setAt(size_t idx, void *value)
{
  if(idx < m_Count)
    m_Data[idx] = value;
}
void Vector<void*>::clear()
{
  m_Count = 0;
  m_Size = 0;
  delete [] m_Data;
  m_Data = 0;
}
Vector<void*>::Iterator Vector<void*>::erase(Iterator iter)
{
  memmove(iter,
          iter+1,
          reinterpret_cast<uintptr_t>(m_Data) + (m_Count-1)*sizeof(void*) - reinterpret_cast<uintptr_t>(iter));
  m_Count--;
  return iter;
}
void Vector<void*>::assign(const Vector &x)
{
  reserve(x.size(), false);

  memcpy(m_Data, x.m_Data, x.count() * sizeof(void*));
  m_Count = x.count();
}
void Vector<void*>::reserve(size_t size, bool copy)
{
  if (size <= m_Size)
    return;
  else if (size < (m_Size * 2))
  {
    // Grow exponentially.
    size = m_Size * 2;
  }

  void **tmp = m_Data;
  m_Data = new void*[size];
  if (tmp != 0)
  {
    if (copy == true)
      memcpy(m_Data, tmp, m_Size * sizeof(void*));
    delete []tmp;
  }
  m_Size = size;
}

//
// Explicitly instantiate Vector<void*>
//
template class Vector<void*>;
