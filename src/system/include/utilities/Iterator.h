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

/** @addtogroup kernelutilities
 * @{ */

template<class T1, class T2>
bool operator != (const T1 &x1, const T2 &x2)
{
  return !(x1 == x2);
}
template<class T>
T operator ++ (T &x, int)
{
  T tmp(x);
  ++x;
  return tmp;
}
template<class T>
T operator -- (T &x, int)
{
  T tmp(x);
  --x;
  return tmp;
}

template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)() = &Struct::previous,
         Struct *(Struct::*FunctionNext)() = &Struct::next,
         typename T = originalT>
class Iterator
{
  template<typename originalT2,
           class Struct2,
           Struct2 *(Struct2::*FunctionPrev2)(),
           Struct2 *(Struct2::*FunctionNext2)(),
           typename T2>
  friend class Iterator;

  public:
    typedef Iterator<originalT, Struct, FunctionPrev, FunctionNext, T const>     Const;
    typedef Iterator<originalT, Struct, FunctionNext, FunctionPrev, T>           Reverse;
    typedef Iterator<originalT, Struct, FunctionNext, FunctionPrev, T const>     ConstReverse;

    Iterator()
      : m_Node(){}
    Iterator(const Iterator &x)
      : m_Node(x.m_Node){}
    Iterator(Struct *Node)
      : m_Node(Node){}
    ~Iterator(){}

    Iterator &operator = (const Iterator &x)
    {
      m_Node = x.m_Node;
      return *this;
    }
    bool operator == (const Const &x) const
    {
      if (m_Node != x.m_Node)return false;
      return true;
    }
    Iterator &operator ++ ()
    {
      m_Node = (m_Node->*FunctionNext)();
      return *this;
    }
    Iterator &operator -- ()
    {
      m_Node = (m_Node->*FunctionPrev)();
      return *this;
    }
    T &operator *()
    {
      return m_Node->value;
    }

    operator Iterator<originalT, Struct, FunctionPrev, FunctionNext, T const> ()
    {
      return Iterator<originalT, Struct, FunctionPrev, FunctionNext, T const>(m_Node);
    }

  private:
    Struct *m_Node;
};

/** @} */

#endif
