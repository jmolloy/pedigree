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
#ifndef KERNEL_UTILITIES_LIST_H
#define KERNEL_UTILITIES_LIST_H

#include <processor/types.h>
#include <utilities/IteratorAdapter.h>

/** @addtogroup kernelutilities utilities
 * utilities
 *  @ingroup kernel
 * @{ */

template<class T>
class List;

/** List template specialisation for void* */
template<>
class List<void*>
{
  public:
    class Iterator;
    class ConstIterator;
    /** Iterator needs access to List<void*>::node_t */
    friend class Iterator;
    /** ConstIterator needs access to List<void*>::node_t */
    friend class ConstIterator;

  private:
    /** One node in the list */
    struct node_t
    {
      /** Pointer to the next node */
      node_t *next;
      /** Pointer to the previous node */
      node_t *previous;
      /** The value of the node */
      void *value;
    };

  public:
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

  public:
    /** Forward iterator */
    class Iterator
    {
      /** Needed for the copy-constructor */
      friend class ConstIterator;
      public:
        /** Default constructor */
        inline Iterator()
          : m_Node(0){}
        /** Copy-constructor
         *\param[in] x reference object */
        inline Iterator(const Iterator &x)
          : m_Node(x.m_Node){}
        /** Construct Iterator from a pointer to a List<void*>::node_t
         *\param[in] node pointer to the List<void*> node */
        inline Iterator(node_t *node)
          : m_Node(node){}
        /** The destructor */
        inline ~Iterator()
          {}

        /** Assignment operator
         *\param[in] x reference object */
        inline Iterator &operator = (const Iterator &x)
        {
          m_Node = x.m_Node;
          return *this;
        }
        /** Compare Iterator with ConstIterator
         *\param[in] x reference object
         *\return true, if they refer to the same element, false otherwise */
        inline bool operator == (const ConstIterator &x) const
        {
          if (m_Node != x.m_Node)return false;
          return true;
        }
        /** Compare Iterator with ConstIterator
         *\param[in] x reference object
         *\return false, if they refer to the same element, true otherwise */
        inline bool operator != (const ConstIterator &x) const
        {
          if (m_Node == x.m_Node)return false;
          return true;
        }
        /** Compare two Iterators
         *\param[in] x reference object
         *\return true, if they refer to the same element, false otherwise */
        inline bool operator == (const Iterator &x) const
        {
          if (m_Node != x.m_Node)return false;
          return true;
        }
        /** Compare two Iterators
         *\param[in] x reference object
         *\return false, if they refer to the same element, true otherwise */
        inline bool operator != (const Iterator &x) const
        {
          if (m_Node == x.m_Node)return false;
          return true;
        }
        /** Go to the next element in the List
         *\return reference to this iterator */
        inline Iterator &operator ++ ()
        {
          m_Node = m_Node->next;
          return *this;
        }
        /** Go to the next element and return a copy of the old iterator
         *\return the iterator previous to the increment */
        inline Iterator operator ++ (int)
        {
          Iterator tmp(m_Node);
          m_Node = m_Node->next;
          return tmp;
        }
        /** Dereference the iterator, aka get the element
         *\return the element the iterator points to */
        inline void *&operator *()
        {
          return m_Node->value;
        }

      private:
        /** Pointer to the List<void*> node of the element the iterator is
         *  pointing to */
        node_t *m_Node;
    };

    /** Constant forward iterator */
    class ConstIterator
    {
      /** Needed for the comparison operator */
      friend class Iterator;
      public:
        /** Default constructor */
        inline ConstIterator()
          : m_Node(0){}
        /** Copy-constructor
         *\param[in] x reference object */
        inline ConstIterator(const ConstIterator &x)
          : m_Node(x.m_Node){}
        /** Copy-constructor
         *\param[in] x reference object */
        inline ConstIterator(const Iterator &x)
          : m_Node(x.m_Node){}
        /** Construct Iterator from a pointer to a List<void*>::node_t
         *\param[in] node pointer to the List<void*> node */
        inline ConstIterator(node_t *node)
          : m_Node(node){}
        /** The destructor */
        inline ~ConstIterator()
          {}

        /** Assignment operator
         *\param[in] x reference object */
        inline ConstIterator &operator = (const ConstIterator &x)
        {
          m_Node = x.m_Node;
          return *this;
        }
        /** Assignment operator
         *\param[in] x reference object */
        inline ConstIterator &operator = (const Iterator &x)
        {
          m_Node = x.m_Node;
          return *this;
        }
        /** Compare two ConstIterators
         *\param[in] x reference object
         *\return true, if they refer to the same element, false otherwise */
        inline bool operator == (const ConstIterator &x) const
        {
          if (m_Node != x.m_Node)return false;
          return true;
        }
        /** Compare two ConstIterators
         *\param[in] x reference object
         *\return true, if they refer to the same element, false otherwise */
        inline bool operator != (const ConstIterator &x) const
        {
          if (m_Node == x.m_Node)return false;
          return true;
        }
        /** Go to the next element in the List
         *\return reference to this iterator */
        inline ConstIterator &operator ++ ()
        {
          m_Node = m_Node->next;
          return *this;
        }
        /** Go to the next element and return a copy of the old iterator
         *\return the iterator previous to the increment */
        inline ConstIterator operator ++ (int)
        {
          ConstIterator tmp(m_Node);
          m_Node = m_Node->next;
          return tmp;
        }
        /** Dereference the iterator, aka get the element
         *\return the element the iterator points to */
        inline void *operator *()
        {
          return m_Node->value;
        }

      private:
        /** Pointer to the List<void*> node of the element the iterator is
         *  pointing to */
        node_t *m_Node;
    };
};

/** List template specialisation for pointers. Just forwards to the
 * void* template specialisation of List. */
template<class T>
class List<T*>
{
  public:
    /** Iterator */
    typedef IteratorAdapter<T*, List<void*>::Iterator>             Iterator;
    /** ConstIterator */
    typedef IteratorAdapter<T* const, List<void*>::ConstIterator>  ConstIterator;

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
      m_VoidList.pushBack(reinterpret_cast<void*>(value));
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
