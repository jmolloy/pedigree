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
      : m_List(), m_bReverse(false), m_bPreferUsed(false) {}
    /** Construct with reverse order, without an initial allocation. */
    inline RangeList(bool reverse, bool preferUsed = false)
      : m_List(), m_bReverse(reverse), m_bPreferUsed(preferUsed) {}
    /** Construct with a preexisting range
     *\param[in] Address beginning of the range
     *\param[in] Length length of the range */
    RangeList(T Address, T Length, bool bReverse = false, bool preferUsed = false)
      : m_List(), m_bReverse(bReverse), m_bPreferUsed(preferUsed)
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

    /** Sweep the RangeList and re-merge items. */
    void sweep();

  private:

    /** List of ranges */
    List<Range*> m_List;

    /** Should we allocate in reverse order? */
    bool m_bReverse;

    /** Should we prefer previously-used ranges where possible? */
    bool m_bPreferUsed;

    RangeList &operator = (const RangeList & l);

    typedef typename List<Range*>::Iterator Iterator;
    typedef typename List<Range*>::ConstIterator ConstIterator;
    typedef typename List<Range*>::ReverseIterator ReverseIterator;
    typedef typename List<Range*>::ConstReverseIterator ConstReverseIterator;
};

/** @} */

/** Copy constructor - performs deep copy. */
template<typename T>
RangeList<T>::RangeList(const RangeList<T>& other)
  : m_List()
{
    m_List.clear();
    m_bReverse = other.m_bReverse;

    for (Iterator it = other.m_List.begin(); it != other.m_List.end(); ++it)
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

  // Try and find a place to merge immediately.
  bool needsNew = true;
  for (; cur != end; ++cur)
  {
    // Region ends at our freed address.
    if (((*cur)->address + (*cur)->length) == address)
    {
      // Update - all done.
      (*cur)->length += length;
      needsNew = false;
      break;
    }
    // Region starts after our address.
    else if ((*cur)->address == (address + length))
    {
      // Expand.
      (*cur)->address -= length;
      (*cur)->length += length;
      needsNew = false;
      break;
    }
  }

  // Clean up any merges that we may have just created.
  sweep();

  if (!needsNew)
    return;

  // Couldn't find a merge, so we need to add a new region.

  // Add the range back to our list, but in such a way that it is allocated
  // last rather than first (if another allocation of the same length comes
  // later).
  Range *range = new Range(address, length);

  // Decide which side of the list to push to. If we prefer used ranges over
  // fresh ranges, we want to invert the push decision.
  bool front = m_bReverse;
  if (m_bPreferUsed) front = !front;

  if (front)
    m_List.pushFront(range);
  else
    m_List.pushBack(range);
}

template<typename T>
bool RangeList<T>::allocate(T length, T &address)
{
    bool bSuccess = false;

    // Try and find enough space. This logic differs slightly if we are working
    // in reverse, as the direction changes.
    if (m_bReverse)
    {
        for (ReverseIterator it = m_List.rbegin(); it != m_List.rend(); ++it)
        {
            if ((*it)->length >= length)
            {
                // Big enough. Cut into the END of this range.
                T offset = (*it)->length - length;
                address = (*it)->address + offset;
                (*it)->length -= length;

                // Remove if the entry no longer exists.
                if (!(*it)->length)
                {
                    delete (*it);
                    m_List.erase(it);
                }

                bSuccess = true;
                break;
            }
        }
    }
    else
    {
        for (Iterator it = m_List.begin(); it != m_List.end(); ++it)
        {
            if ((*it)->length >= length)
            {
                // Big enough. Cut into the START of this range.
                address = (*it)->address;
                (*it)->address += length;
                (*it)->length -= length;

                // Remove if the entry no longer exists.
                if (!(*it)->length)
                {
                    delete (*it);
                    m_List.erase(it);
                }

                bSuccess = true;
                break;
            }
        }
    }

    // Now that we've possibly removed items or rearranged them, re-sweep.
    sweep();
    return bSuccess;
}

template<typename T>
bool RangeList<T>::allocateSpecific(T address, T length)
{
    bool bSuccess = false;
    for (Iterator cur = m_List.begin(); cur != m_List.end(); ++cur)
    {
        // Precise match.
        if ((*cur)->address == address && (*cur)->length == length)
        {
            delete *cur;
            m_List.erase(cur);
            bSuccess = true;
            break;
        }

        // Match at end.
        else if ((*cur)->address < address && ((*cur)->address + (*cur)->length) == (address + length))
        {
            (*cur)->length -= length;
            bSuccess = true;
            break;
        }

        // Match at start.
        else if ((*cur)->address == address && (*cur)->length > length)
        {
            (*cur)->address += length;
            (*cur)->length -= length;
            bSuccess = true;
            break;
        }

        // Match within.
        else if ((*cur)->address < address && ((*cur)->address + (*cur)->length) > (address + length))
        {
            // Need to split the range.
            Range *newRange = new Range(address + length, (*cur)->address + (*cur)->length - address - length);
            m_List.pushBack(newRange);
            (*cur)->length = address - (*cur)->address;
            bSuccess = true;
        }

    }

    sweep();
    return bSuccess;
}

template<typename T>
typename RangeList<T>::Range RangeList<T>::getRange(size_t index) const
{
  if (index >= m_List.size()) return Range(0, 0);

  ConstIterator cur(m_List.begin());
  for (size_t i = 0; i < index; ++i) ++cur;
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
    for (ConstIterator it = m_List.begin(); it != m_List.end(); ++it)
        delete *it;
    m_List.clear();
}

template<typename T>
void RangeList<T>::sweep()
{
  // Try and clean up, merging as needed.
  for (Iterator cur = m_List.begin(); cur != m_List.end(); ++cur)
  {
    // Can we merge? (note: preincrement modifies the iterator)
    Iterator next = cur;
    ++next;
    if (next == m_List.end())
      break;

    uintptr_t cur_address = (*cur)->address;
    uintptr_t next_address = (*next)->address;
    size_t cur_len = (*cur)->length;
    size_t next_len = (*next)->length;

    if ((cur_address + cur_len) == next_address)
    {
      // Merge.
      (*cur)->length += next_len;
      delete *next;
      m_List.erase(next);
    }
    else if ((next_address + next_len) == cur_address)
    {
      (*cur)->address -= next_len;
      (*cur)->length += next_len;
      delete *next;
      m_List.erase(next);
    }
  }
}

#endif
