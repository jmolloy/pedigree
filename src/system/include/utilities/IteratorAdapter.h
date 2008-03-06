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
#ifndef KERNEL_UTILITIES_ITERATORADAPTER_H
#define KERNEL_UTILITIES_ITERATORADAPTER_H

#include <processor/types.h>

/** @addtogroup kernelutilities
 * @{ */

/** Adapter for an iterator with a different element type */
template<typename T, class Iterator>
class IteratorAdapter
{
  public:
    /** Default constructor */
    inline IteratorAdapter()
      : m_Iterator(){}
    /** Construct form an Iterator
     *\param[in] x the Iterator reference object */
    inline IteratorAdapter(const Iterator &x)
      : m_Iterator(x){}
    /** Construct from another iterator compatible with Iterator
     *\param[in] x the reference object */
    template<typename T2, class Iterator2>
    inline IteratorAdapter(const IteratorAdapter<T2, Iterator2> &x)
      : m_Iterator(x.__getIterator()){}
    /** Copy-constructor
     *\param[in] x reference object */
    inline IteratorAdapter(const IteratorAdapter &x)
      : m_Iterator(x.m_Iterator){}
    /** The destructor */
    inline ~IteratorAdapter()
      {}

    /** Assignment operator
     *\param[in] x reference object */
    inline IteratorAdapter &operator = (const IteratorAdapter &x)
    {
      m_Iterator = x.m_Iterator;
      return *this;
    }
    /** Assign from another iterator compatible with Iterator
     *\param[in] x the reference object */
    template<typename T2, class Iterator2>
    inline IteratorAdapter &operator = (const IteratorAdapter<T2, Iterator2> &x)
    {
      m_Iterator = x.__getIterator();
      return *this;
    }
    /** Comparison operator
     *\param[in] x reference object
     *\return true, if the iterators point to the same object, false otherwise */
    inline bool operator == (const IteratorAdapter &x) const
    {
      if (m_Iterator != x.m_Iterator)return false;
      return true;
    }
    /** Comparison operator
     *\param[in] x reference object
     *\return true, if the iterators don't point to the same object, false otherwise */
    inline bool operator != (const IteratorAdapter &x) const
    {
      if (m_Iterator == x.m_Iterator)return false;
      return true;
    }
    /** Go to the next element in the List
     *\return reference to this iterator */
    inline IteratorAdapter &operator ++ ()
    {
      ++m_Iterator;
      return *this;
    }
    /** Go to the next element and return a copy of the old iterator
     *\return the iterator previous to the increment */
    inline IteratorAdapter operator ++ (int)
    {
      IteratorAdapter tmp(*this);
      ++m_Iterator;
      return tmp;
    }
    /** Dereference the iterator, aka get the element
     *\return the element the iterator points to */
    inline T &operator *()
    {
      return reinterpret_cast<T>(*m_Iterator);
    }

    /** Get a reference to the iterator
     *\note not supposed to be used from the outside world! */
    inline const Iterator &__getIterator() const
    {
      return m_Iterator;
    }

  private:
    /** The adapted iterator */
    Iterator m_Iterator;
};

/** @} */

#endif
