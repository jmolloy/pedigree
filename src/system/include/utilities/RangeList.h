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

#ifndef KERNEL_UTILITIES_RANGELIST_H
#define KERNEL_UTILITIES_RANGELIST_H

#include <utilities/List.h>
#include <Log.h>

/** @addtogroup kernelutilities
 * @{ */

/** This class manages a List of ranges. It automatically merges adjacent entries
 *  in the list.
 *\param[in] T the integer type the range address and length is encoded in */
template<typename T>
class RangeList
{
  public:
    /** Default constructor does nothing */
    inline RangeList()
      : m_List(), m_bReverse(false) {}
    /** Construct with reverse order, without an initial allocation. */
    inline RangeList(bool reverse)
      : m_List(), m_bReverse(reverse) {}
    /** Construct with a preexisting range
     *\param[in] Address beginning of the range
     *\param[in] Length length of the range */
    RangeList(T Address, T Length, bool bReverse = false)
      : m_List(), m_bReverse(bReverse)
    {
      Range *range = new Range(Address, Length);
      m_List.pushBack(range);
    }
    /** Destructor frees the list */
    ~RangeList();

    RangeList(const RangeList&);

    /** Structure of one range */
    class Range
    {
    public:
      /** Construct a Range */
      inline Range(T Address, T Length)
          : address(Address), length(Length) {}

      /** Beginning address of the range */
      T address;
      /** Length of the range  */
      T length;
    };

    /** Free a range
     *\param[in] address beginning address of the range
     *\param[in] length length of the range */
    void free(T address, T length);
    /** Allocate a range of a specific size
     *\param[in] length the requested length
     *\param[in,out] address the beginning address of the allocated range
     *\return true, if successfully allocated (and address is valid), false otherwise */
    bool allocate(T length, T &address);
    /** Allocate a range of specific size and beginning address
     *\param[in] address the beginning address
     *\param[in] length the length
     *\return true, if successfully allocated, false otherwise */
    bool allocateSpecific(T address, T length);
    void clear();

    /** Get the number of ranges in the list
     *\return the number of ranges in the list */
    inline size_t size() const{return m_List.size();}
    /** Get a range at a specific index */
    Range getRange(size_t index) const;

  private:

    /** List of ranges */
    List<Range*> m_List;

    /** Should we allocate in reverse order? */
    bool m_bReverse;

    RangeList &operator = (const RangeList & l);

    typedef typename List<Range*>::Iterator Iterator;
    typedef typename List<Range*>::ConstIterator ConstIterator;
    typedef typename List<Range*>::ReverseIterator ReverseIterator;
    typedef typename List<Range*>::ConstReverseIterator ConstReverseIterator;
};

/** @} */

//
// The implementation
//
/** Copy constructor - performs deep copy. */
template<typename T>
RangeList<T>::RangeList(const RangeList<T>& other)
  : m_List()
{
    m_List.clear();
    Iterator it(other.m_List.begin());
    for (;
        it != other.m_List.end();
        it++)
    {
        Range *pRange = new Range((*it)->address, (*it)->length);
        m_List.pushBack(pRange);
    }
}

template<typename T>
void RangeList<T>::free(T address, T length)
{
  Iterator cur(m_List.begin());
  ConstIterator end(m_List.end());
  for (;cur != end;++cur)
    if (((*cur)->address + (*cur)->length) == address)
    {
      address = (*cur)->address;
      length += (*cur)->length;
      delete *cur;
      m_List.erase(cur);
      break;
    }

  cur = m_List.begin();
  end = m_List.end();
  for (;cur != end;++cur)
    if ((*cur)->address == (address + length))
    {
      length += (*cur)->length;
      delete *cur;
      m_List.erase(cur);
      break;
    }

  Range *range = new Range(address, length);
  m_List.pushBack(range);
}
template<typename T>
bool RangeList<T>::allocate(T length, T &address)
{
  if(m_bReverse)
  {
    ReverseIterator cur(m_List.rbegin());
    ConstReverseIterator end(m_List.rend());
    for (;cur != end;++cur)
      if ((*cur)->length >= length)
      {
        T offset = (*cur)->length - length;
        address = (*cur)->address + offset;
        (*cur)->length -= length;
        if ((*cur)->length == 0)
        {
          delete *cur;
          m_List.erase(cur);
        }
        return true;
      }
  }
  else
  {
    Iterator cur(m_List.begin());
    ConstIterator end(m_List.end());
    for (;cur != end;++cur)
      if ((*cur)->length >= length)
      {
        address = (*cur)->address;
        (*cur)->address += length;
        (*cur)->length -= length;
        if ((*cur)->length == 0)
        {
          delete *cur;
          m_List.erase(cur);
        }
        return true;
      }
  }
  return false;
}
template<typename T>
bool RangeList<T>::allocateSpecific(T address, T length)
{
  Iterator cur(m_List.begin());
  ConstIterator end(m_List.end());
  for (;cur != end;++cur)
    if ((*cur)->address == address &&
        (*cur)->length == length)
    {
      delete *cur;
      m_List.erase(cur);
      return true;
    }
    else if ((*cur)->address == address &&
             (*cur)->length > length)
    {
      (*cur)->address += length;
      (*cur)->length -= length;
      return true;
    }
    else if ((*cur)->address < address &&
             ((*cur)->address + (*cur)->length) == (address + length))
    {
      (*cur)->length -= length;
      return true;
    }
    else if ((*cur)->address < address &&
             ((*cur)->address + (*cur)->length) > (address + length))
    {
      Range *newRange = new Range(address + length, (*cur)->address + (*cur)->length - address - length);
      m_List.pushBack(newRange);
      (*cur)->length = address - (*cur)->address;
      return true;
    }
  return false;
}
template<typename T>
typename RangeList<T>::Range RangeList<T>::getRange(size_t index) const
{
  if (index >= m_List.size())return Range(0, 0);

  ConstIterator cur(m_List.begin());
  for (size_t i = 0;i < index;++i)++cur;
  return Range(**cur);
}
template<typename T>
RangeList<T>::~RangeList()
{
    clear();
}
template<typename T>
void RangeList<T>::clear()
{
    ConstIterator cur(m_List.begin());
    ConstIterator end(m_List.end());
    for (;cur != end;++cur)
        delete *cur;
    m_List.clear();
}
#endif
