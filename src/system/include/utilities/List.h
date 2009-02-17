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

#ifndef KERNEL_UTILITIES_LIST_H
#define KERNEL_UTILITIES_LIST_H

#include <utilities/template.h>
#include <processor/types.h>
#include <utilities/IteratorAdapter.h>
#include <utilities/Iterator.h>
#include <Log.h>
/** @addtogroup kernelutilities
 * @{ */

/** One node in the list
 *\brief One node in the list
 *\param[in] T the element type */
template<typename T>
struct _ListNode_t
{
  /** Get the next data structure in the list
   *\return pointer to the next data structure in the list */
  _ListNode_t *next()
    {return m_Next;}
  /** Get the previous data structure in the list
   *\return pointer to the previous data structure in the list */
  _ListNode_t *previous()
    {return m_Previous;}

  /** Pointer to the next node */
  _ListNode_t *m_Next;
  /** Pointer to the previous node */
  _ListNode_t *m_Previous;
  /** The value of the node */
  T value;
};

template<class T>
class List;

/** List template specialisation for void*
 *\brief List template specialisation for void* */
template<>
class List<void*>
{
  /** The data structure of the list's nodes */
  typedef _ListNode_t<void*> node_t;
  public:
    /** Type of the bidirectional iterator */
    typedef ::Iterator<void*, node_t>     Iterator;
    /** Type of the constant bidirectional iterator */
    typedef Iterator::Const               ConstIterator;
    /** Type of the reverse iterator */
    typedef Iterator::Reverse             ReverseIterator;
    /** Type of the constant reverse iterator */
    typedef Iterator::ConstReverse        ConstReverseIterator;

    /** Default constructor, does nothing */
    List();
    /** Copy-constructor
     *\param[in] x reference object */
    List(const List &x);
    /** Destructor, deallocates memory */
    ~List();

    /** Assignment operator
     *\param[in] x the object that should be copied */
    List &operator = (const List &x);

    /** Get the number of elements we reserved space for
     *\return number of elements we reserved space for */
    size_t size() const;
    /** Get the number of elements in the List */
    size_t count() const;
    /** Add a value to the end of the List
     *\param[in] value the value that should be added */
    void pushBack(void *value);
    /** Remove the last element from the List
     *\return the previously last element */
    void *popBack();
    /** Add a value to the front of the List
     *\param[in] value the value that should be added */
    void pushFront(void *value);
    /** Remove the first element in the List
     *\return the previously first element */
    void *popFront();
    /** Erase an element
     *\param[in] iterator the iterator that points to the element */
    Iterator erase(Iterator &Iter);

    /** Get an iterator pointing to the beginning of the List
     *\return iterator pointing to the beginning of the List */
    inline Iterator begin()
    {
      return Iterator(m_First);
    }
    /** Get a constant iterator pointing to the beginning of the List
     *\return constant iterator pointing to the beginning of the List */
    inline ConstIterator begin() const
    {
      return ConstIterator(m_First);
    }
    /** Get an iterator pointing to the end of the List + 1
     *\return iterator pointing to the end of the List + 1 */
    inline Iterator end()
    {
      return Iterator();
    }
    /** Get a constant iterator pointing to the end of the List + 1
     *\return constant iterator pointing to the end of the List + 1 */
    inline ConstIterator end() const
    {
      return ConstIterator();
    }
    /** Get an iterator pointing to the reverse beginning of the List
     *\return iterator pointing to the reverse beginning of the List */
    inline ReverseIterator rbegin()
    {
      return ReverseIterator(m_Last);
    }
    /** Get a constant iterator pointing to the reverse beginning of the List
     *\return constant iterator pointing to the reverse beginning of the List */
    inline ConstReverseIterator rbegin() const
    {
      return ConstReverseIterator(m_Last);
    }
    /** Get an iterator pointing to the reverse end of the List + 1
     *\return iterator pointing to the reverse end of the List + 1 */
    inline ReverseIterator rend()
    {
      return ReverseIterator();
    }
    /** Get a constant iterator pointing to the reverse end of the List + 1
     *\return constant iterator pointing to the reverse end of the List + 1 */
    inline ConstReverseIterator rend() const
    {
      return ConstReverseIterator();
    }

    /** Remove all elements from the List */
    void clear();
    /** Copy the content of a List into this List
     *\param[in] x the reference List */
    void assign(const List &x);

  private:
    /** The number of Nodes/Elements in the List */
    size_t m_Count;
    /** Pointer to the first Node in the List */
    node_t *m_First;
    /** Pointer to the last Node in the List */
    node_t *m_Last;
};

/** List template specialisation for pointers. Just forwards to the
 * void* template specialisation of List.
 *\brief List template specialisation for pointers */
template<class T>
class List<T*>
{
  public:
    /** Iterator */
    typedef IteratorAdapter<T*, List<void*>::Iterator>                    Iterator;
    /** ConstIterator */
    typedef IteratorAdapter<T* const, List<void*>::ConstIterator>         ConstIterator;
    /** ReverseIterator */
    typedef IteratorAdapter<T*, List<void*>::ReverseIterator>             ReverseIterator;
    /** ConstReverseIterator */
    typedef IteratorAdapter<T* const, List<void*>::ConstReverseIterator>  ConstReverseIterator;

    /** Default constructor, does nothing */
    inline List()
      : m_VoidList(){}
    /** Copy-constructor
     *\param[in] x reference object */
    inline List(const List &x)
      : m_VoidList(x.m_VoidList){}
    /** Destructor, deallocates memory */
    inline ~List()
      {}

    /** Assignment operator
     *\param[in] x the object that should be copied */
    inline List &operator = (const List &x)
    {
      m_VoidList = x.m_VoidList;
      return *this;
    }

    /** Get the number of elements we reserved space for
     *\return number of elements we reserved space for */
    inline size_t size() const
    {
      return m_VoidList.size();
    }
    /** Get the number of elements in the List */
    inline size_t count() const
    {
      return m_VoidList.count();
    }
    /** Add a value to the end of the List
     *\param[in] value the value that should be added */
    inline void pushBack(T *value)
    {
      m_VoidList.pushBack(reinterpret_cast<void*>(const_cast<typename nonconst_type<T>::type*>(value)));
    }
    /** Remove the last element from the List
     *\return the previously last element */
    inline T *popBack()
    {
      return reinterpret_cast<T*>(m_VoidList.popBack());
    }
    /** Add a value to the front of the List
     *\param[in] value the value that should be added */
    inline void pushFront(T *value)
    {
      m_VoidList.pushFront(reinterpret_cast<void*>(value));
    }
    /** Remove the first element in the List
     *\return the previously first element */
    inline T *popFront()
    {
      return reinterpret_cast<T*>(m_VoidList.popFront());
    }
    /** Erase an element
     *\param[in] iterator the iterator that points to the element */
    inline Iterator erase(Iterator &Iter)
    {
      return Iterator(m_VoidList.erase(Iter.__getIterator()));
    }

    /** Get an iterator pointing to the beginning of the List
     *\return iterator pointing to the beginning of the List */
    inline Iterator begin()
    {
      return Iterator(m_VoidList.begin());
    }
    /** Get a constant iterator pointing to the beginning of the List
     *\return constant iterator pointing to the beginning of the List */
    inline ConstIterator begin() const
    {
      return ConstIterator(m_VoidList.begin());
    }
    /** Get an iterator pointing to the end of the List + 1
     *\return iterator pointing to the end of the List + 1 */
    inline Iterator end()
    {
      return Iterator(m_VoidList.end());
    }
    /** Get a constant iterator pointing to the end of the List + 1
     *\return constant iterator pointing to the end of the List + 1 */
    inline ConstIterator end() const
    {
      return ConstIterator(m_VoidList.end());
    }
    /** Get an iterator pointing to the reverse beginning of the List
     *\return iterator pointing to the reverse beginning of the List */
    inline ReverseIterator rbegin()
    {
      return ReverseIterator(m_VoidList.rbegin());
    }
    /** Get a constant iterator pointing to the reverse beginning of the List
     *\return constant iterator pointing to the reverse beginning of the List */
    inline ConstReverseIterator rbegin() const
    {
      return ConstReverseIterator(m_VoidList.rbegin());
    }
    /** Get an iterator pointing to the reverse end of the List + 1
     *\return iterator pointing to the reverse end of the List + 1 */
    inline ReverseIterator rend()
    {
      return ReverseIterator(m_VoidList.rend());
    }
    /** Get a constant iterator pointing to the reverse end of the List + 1
     *\return constant iterator pointing to the reverse end of the List + 1 */
    inline ConstReverseIterator rend() const
    {
      return ConstReverseIterator(m_VoidList.rend());
    }

    /** Remove all elements from the List */
    inline void clear()
    {
      m_VoidList.clear();
    }
    /** Copy the content of a List into this List
     *\param[in] x the reference List */
    inline void assign(const List &x)
    {
      m_VoidList.assign(x.m_VoidList);
    }

  private:
    /** The actual container */
    List<void*> m_VoidList;
};

/** @} */

#endif
