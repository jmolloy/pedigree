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
#ifndef KERNEL_UTILITIES_RANGELIST_H
#define KERNEL_UTILITIES_RANGELIST_H

#include <utilities/List.h>

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
      : m_List(){}
    /** Construct with a preexisting range
     *\param[in] Address beginning of the range
     *\param[in] Length length of the range */
    RangeList(T Address, T Length)
      : m_List()
    {
      Range *range = new Range(Address, Length);
      m_List.pushBack(range);
    }
    /** Destructor frees the list */
    ~RangeList();

    /** Structure of one range */
    struct Range
    {
      /** Construct a Range */
      inline Range(T Address, T Length)
        : address(Address), length(Length){}

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

    /** Get the number of ranges in the list
     *\return the number of ranges in the list */
    inline size_t size() const{return m_List.size();}
    /** Get a range at a specific index */
    Range getRange(size_t index) const;

  private:
    RangeList(const RangeList &);
    RangeList &operator = (const RangeList &);

    /** List of ranges */
    List<Range*> m_List;
};

/** @} */

//
// The implementation
//
template<typename T>
void RangeList<T>::free(T address, T length)
{
  typename List<Range*>::Iterator cur(m_List.begin());
  typename List<Range*>::ConstIterator end(m_List.end());
  for (;cur != end;++cur)
    if ((cur->address + cur->length) == address)
    {
      address = cur->address;
      length += cur->length;
      delete *cur;
      m_List.erase(cur);
    }

  cur = m_List.begin();
  end = m_List.end();
  for (;cur != end;++cur)
    if (cur->address == (address + length))
    {
      length += cur->length;
      delete *cur;
      m_List.erase(cur);
    }

  Range *range = new Range(address, length);
  m_List.pushBack(range);
}
template<typename T>
bool RangeList<T>::allocate(T length, T &address)
{
  typename List<Range*>::Iterator cur(m_List.begin());
  typename List<Range*>::ConstIterator end(m_List.end());
  for (;cur != end;++cur)
    if (cur->length >= length)
    {
      address = cur->address;
      cur->address += length;
      cur->length -= length;
      if (cur->length == 0)
      {
        delete *cur;
        m_List.erase(cur);
      }
      return true;
    }
  return false;
}
template<typename T>
bool RangeList<T>::allocateSpecific(T address, T length)
{
  typename List<Range*>::Iterator cur(m_List.begin());
  typename List<Range*>::ConstIterator end(m_List.end());
  for (;cur != end;++cur)
    if (cur->address == address &&
        cur->length == length)
    {
      delete *cur;
      m_List.erase(cur);
      return true;
    }
    else if (cur->address == address &&
             cur->length > length)
    {
      cur->address += length;
      cur->length -= length;
      return true;
    }
    else if (cur->address < address &&
             (cur->address + cur->length) == (address + length))
    {
      cur->length -= length;
      return true;
    }
    else if (cur->address < address &&
             (cur->address + cur->length) > (address + length))
    {
      Range *newRange = new Range(address + length, cur->address + cur->length - address - length);
      m_List.pushBack(newRange);
      cur->length = address - cur->address;
      return true;
    }
  return false;
}
template<typename T>
typename RangeList<T>::Range RangeList<T>::getRange(size_t index) const
{
  if (index >= m_List.size())return Range(0, 0);

  typename List<Range*>::ConstIterator cur(m_List.begin());
  for (size_t i = 0;i < index;++i)++cur;
  return Range(**cur);
}
template<typename T>
RangeList<T>::~RangeList()
{
  typename List<Range*>::ConstIterator cur(m_List.begin());
  typename List<Range*>::ConstIterator end(m_List.end());
  for (;cur != end;++cur)
    delete *cur;
}

#endif
