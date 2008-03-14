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
#ifndef KERNEL_UTILITIES_VECTOR_H
#define KERNEL_UTILITIES_VECTOR_H

#include <processor/types.h>

/** @addtogroup kernelutilities
 * @{ */

/** General Vector template class, aka dynamic array
 *\brief A vector / dynamic array */
template<class T>
class Vector;

/** Vector template specialisation for void*
 *\brief A vector / dynamic array for void* */
template<>
class Vector<void*>
{
  public:
    /** Random-access iterator for the Vector */
    typedef void**        Iterator;
    /** Contant random-access iterator for the Vector */
    typedef void* const*  ConstIterator;

    /** The default constructor, does nothing */
    Vector();
    /** Reserves space for size elements
     *\param[in] size the number of elements */
    explicit Vector(size_t size);
    /** The copy-constructor
     *\param[in] x the reference object to copy */
    Vector(const Vector &x);
    /** The destructor, deallocates memory */
    ~Vector();

    /** The assignment operator
     *\param[in] x the object that should be copied */
    Vector &operator = (const Vector &x);
    /** The [] operator
     *\param[in] index the index of the element that should be returned
     *\return the element at index index */
    void *operator [](size_t index) const;

    /** Get the number of elements that we have reserved space for
     *\return the number of elements that we have reserved space for */
    size_t size() const;
    /** Get the number of elements in the Vector
     *\return the number of elements in the Vector */
    size_t count() const;
    /** Add an element to the end of the Vector
     *\param[in] value the element */
    void pushBack(void *value);
    /** Remove the element from the back and return it
     *\return the removed element */
    void *popBack();

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    inline Iterator begin()
    {
      return m_Data;
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    inline ConstIterator begin() const
    {
      return m_Data;
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    inline Iterator end()
    {
      return m_Data + m_Count;
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    inline ConstIterator end() const
    {
      return m_Data + m_Count;
    }
    /** Copy the content of a Vector into this Vector
     *\param[in] x the reference Vector */
    void assign(const Vector &x);
    /** Reserve space for size elements
     *\param[in] size the number of elements to reserve space for
     *\param[in] copy Should we copy the old contents over? */
    void reserve(size_t size, bool copy);

  private:
    /** The number of elements we have reserved space for */
    size_t m_Size;
    /** The number of elements in the Vector */
    size_t m_Count;
    /** Pointer to the Elements */
    void **m_Data;
};

/** Vector template specialisation for pointers. Just forwards to the
 * void* template specialisation of Vector.
 *\brief A vector / dynamic array for pointer types */
template<class T>
class Vector<T*>
{
  public:
    /** Random-access iterator for the Vector */
    typedef T**        Iterator;
    /** Contant random-access iterator for the Vector */
    typedef T* const*  ConstIterator;

    /** The default constructor, does nothing */
    inline Vector()
      : m_VoidVector(){};
    /** Reserves space for size elements
     *\param[in] size the number of elements */
    inline explicit Vector(size_t size)
      : m_VoidVector(size){}
    /** The copy-constructor
     *\param[in] x the reference object */
    inline Vector(const Vector &x)
      : m_VoidVector(x.m_VoidVector){}
    /** The destructor, deallocates memory */
    inline ~Vector()
      {}

    /** The assignment operator
     *\param[in] x the object that should be copied */
    inline Vector &operator = (const Vector &x)
      {m_VoidVector = x.m_VoidVector;return *this;}
    /** The [] operator
     *\param[in] index the index of the element that should be returned
     *\return the element at index index */
    inline T *operator [](size_t index) const
      {return reinterpret_cast<T*>(m_VoidVector[index]);}

    /** Get the number of elements that we have reserved space for
     *\return the number of elements that we have reserved space for */
    inline size_t size() const
      {return m_VoidVector.size();}
    /** Get the number of elements in the Vector
     *\return the number of elements in the Vector */
    inline size_t count() const
      {return m_VoidVector.count();}
    /** Add an element to the end of the Vector
     *\param[in] value the element */
    inline void pushBack(T *value)
      {m_VoidVector.pushBack(reinterpret_cast<void*>(value));}
    /** Remove the element from the back and return it
     *\return the removed element */
    inline T *popBack()
      {return reinterpret_cast<T*>(m_VoidVector.popBack());}

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    inline Iterator begin()
    {
      return reinterpret_cast<Iterator>(m_VoidVector.begin());
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    inline ConstIterator begin() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidVector.begin());
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    inline Iterator end()
    {
      return reinterpret_cast<Iterator>(m_VoidVector.end());
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    inline ConstIterator end() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidVector.end());
    }

    /** Reserve space for size elements
     *\param[in] size the number of elements to reserve space for
     *\param[in] copy Should we copy the old contents over? */
    inline void reserve(size_t size, bool copy)
      {m_VoidVector.reserve(size, copy);}

  private:
    /** The actual container */
    Vector<void*> m_VoidVector;
};

/** @} */

#endif
