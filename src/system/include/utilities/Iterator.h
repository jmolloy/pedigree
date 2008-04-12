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
#ifndef KERNEL_UTILITIES_ITERATOR_H
#define KERNEL_UTILITIES_ITERATOR_H

#include <utilities/template.h>

/** @addtogroup kernelutilities
 * @{ */

/** General iterator for structures that provide functions for the next and previous structure
 *  in the datastructure and a "value" member. This template provides a bidirectional, a constant
 *  bidirectional, a reverse bidirectional and a constant reverse bidirectional iterator.
 *\brief An iterator applicable for many data structures
 *\param[in] originalT the original element type of the iterator
 *\param[in] Struct the datastructure that provides functions for the next/previous datastructure
 *                  and a "value" member
 *\param[in] previous pointer to the member function used to iterate forward
 *\param[in] next pointer to the member function used to iterate backwards
 *\param[in] T the real element type of the iterator */
template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)() = &Struct::previous,
         Struct *(Struct::*FunctionNext)() = &Struct::next,
         typename T = originalT>
class Iterator
{
  /** All iterators must be friend in order to allow casts between some iterator types */
  template<typename _originalT,
           class _Struct,
           _Struct *(_Struct::*_FunctionPrev)(),
           _Struct *(_Struct::*_FunctionNext)(),
           typename _T>
  friend class Iterator;

  /** The assignment operator is extern */
  template<typename _originalT,
          class _Struct,
          _Struct *(_Struct::*_FunctionPrev)(),
          _Struct *(_Struct::*_FunctionNext)(),
          typename _T1,
          typename _T2>
  friend bool operator == (const Iterator<_originalT, _Struct, _FunctionPrev, _FunctionNext, _T1> &x1,
                           const Iterator<_originalT, _Struct, _FunctionPrev, _FunctionNext, _T2> &x2);

  public:
    /** Type of the constant bidirectional iterator */
    typedef Iterator<originalT, Struct, FunctionPrev, FunctionNext, T const>     Const;
    /** Type of the reverse iterator */
    typedef Iterator<originalT, Struct, FunctionNext, FunctionPrev, T>           Reverse;
    /** Type of the constant reverse iterator */
    typedef Iterator<originalT, Struct, FunctionNext, FunctionPrev, T const>     ConstReverse;

    /** The default constructor constructs an invalid/unusable iterator */
    Iterator()
      : m_Node(){}
    /** The copy-constructor
     *\param[in] Iterator the reference object */
    Iterator(const Iterator &x)
      : m_Node(x.m_Node){}
    /** The constructor
     *\param[in] Iterator the reference object */
    template<typename T2>
    Iterator(const Iterator<originalT, Struct, FunctionPrev, FunctionNext, T2> &x)
      : m_Node(x.m_Node){}
    /** Constructor from a pointer to an instance of the data structure
     *\param[in] Node pointer to an instance of the data structure */
    Iterator(Struct *Node)
      : m_Node(Node){}
    /** The destructor does nothing */
    ~Iterator(){}

    /** The assignment operator
     *\param[in] Iterator the reference object */
    Iterator &operator = (const Iterator &x)
    {
      m_Node = x.m_Node;
      return *this;
    }
    /** Preincrement operator */
    Iterator &operator ++ ()
    {
      m_Node = (m_Node->*FunctionNext)();
      return *this;
    }
    /** Predecrement operator */
    Iterator &operator -- ()
    {
      m_Node = (m_Node->*FunctionPrev)();
      return *this;
    }
    /** Dereference operator yields the element value */
    T &operator *()
    {
      return m_Node->value;
    }
    /** Dereference operator yields the element value */
    T &operator ->()
    {
      return m_Node->value;
    }

    /** Conversion Operator to a constant iterator */
    operator Const ()
    {
      return Const(m_Node);
    }

    /** Get the Node */
    Struct *__getNode()
    {
      return m_Node;
    }

  private:
    /** Pointer to the instance of the data structure or 0 */
    Struct *m_Node;
};

/** Comparison operator for the Iterator class */
template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)(),
         Struct *(Struct::*FunctionNext)(),
         typename T1,
         typename T2>
bool operator == (const Iterator<originalT, Struct, FunctionPrev, FunctionNext, T1> &x1,
                  const Iterator<originalT, Struct, FunctionPrev, FunctionNext, T2> &x2)
{
  if (x1.m_Node != x2.m_Node)return false;
  return true;
}

/** @} */

#endif
