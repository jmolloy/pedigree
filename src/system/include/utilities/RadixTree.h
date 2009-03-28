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

#ifndef KERNEL_UTILITIES_RADIX_TREE_H
#define KERNEL_UTILITIES_RADIX_TREE_H

#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/Iterator.h>
#include <utilities/IteratorAdapter.h>
#include <Log.h>
#include <processor/Processor.h>

/** @addtogroup kernelutilities
 * @{ */

/** Dictionary class, aka Map for string keys. This is
 *  implemented as a Radix Tree - also known as a Patricia Trie.
 * \brief A key/value dictionary for string keys. */
template<class E>
class RadixTree;

/** Tree specialisation for void* */
template<>
class RadixTree<void*>
{
private:
  /** Tree node. */
  class Node
  {
  public:
    Node() :
      key(),value(0),children(0),parent(0),_next(0),prev(0)
    {}

    String key;
    void *value;
    struct Node *children;
    struct Node *parent;
    struct Node *_next;
    struct Node *prev;

    /** Get the next data structure in the list
     *\return pointer to the next data structure in the list */
    Node *next()
    {
      Node *node = this;
      while ((node == this) || (node && (node->value == 0)))
      {
        if (node->children)
          node = node->children;
        else if (node->_next)
          node = node->_next;
        else
        {
          Node *n = node->parent;
          node = 0;
          while (n)
          {
            if (n->_next)
            {
              node = n->_next;
              break;
            }
            n = n->parent;
          }
          continue;
        }
      }
      return node;
    }
    /** Get the previous data structure in the list
     *\return pointer to the previous data structure in the list
     *\note Not implemented! */
    Node *previous()
      {return 0;}
  private:
    Node(const Node&);
    Node &operator =(const Node&);
  };

public:

  /** Type of the bidirectional iterator */
  typedef ::Iterator<void*, Node>       Iterator;
  /** Type of the constant bidirectional iterator */
  typedef Iterator::Const               ConstIterator;

  /** The default constructor, does nothing */
  RadixTree();
  /** The copy-constructor
   *\param[in] x the reference object to copy */
  RadixTree(const RadixTree<void*> &x);
  /** The destructor, deallocates memory */
  ~RadixTree();

  /** The assignment operator
   *\param[in] x the object that should be copied */
  RadixTree &operator = (const RadixTree &x);

  /** Get the number of elements in the Tree
   *\return the number of elements in the Tree */
  size_t count() const;
  /** Add an element to the Tree.
   *\param[in] key the key
   *\param[in] value the element */
  void insert(String key, void *value);
  /** Attempts to find an element with the given key.
   *\return the element found, or NULL if not found. */
  void *lookup(String key);
  /** Attempts to remove an element with the given key. */
  void remove(String key);

  /** Clear the tree */
  void clear();

  /** Get an iterator pointing to the beginning of the List
   *\return iterator pointing to the beginning of the List */
  inline Iterator begin()
  {
    Iterator it(root);
    if (root->value == 0)
      it++;
    return it;
  }
  /** Get a constant iterator pointing to the beginning of the List
   *\return constant iterator pointing to the beginning of the List */
  inline ConstIterator begin() const
  {
    return ConstIterator(root);
  }
  /** Get an iterator pointing to the end of the List + 1
   *\return iterator pointing to the end of the List + 1 */
  inline Iterator end()
  {
    return Iterator(0);
  }
  /** Get a constant iterator pointing to the end of the List + 1
   *\return constant iterator pointing to the end of the List + 1 */
  inline ConstIterator end() const
  {
    return ConstIterator(0);
  }

private:
  /** Internal function to delete a subtree. */
  void deleteNode(Node *node);
  /** Internal function to create a copy of a subtree. */
  Node *cloneNode(Node *node, Node *parent);

  /** Number of items in the tree. */
  int nItems;
  /** The tree's root. */
  Node *root;
};

/** RadixTree template specialisation for pointers. Just forwards to the
 * void* template specialisation of RadixTree.
 *\brief RadixTree template specialisation for pointers */
template<class T>
class RadixTree<T*>
{
  public:
    /** Iterator */
    typedef IteratorAdapter<T*, RadixTree<void*>::Iterator>                    Iterator;
    /** ConstIterator */
    typedef IteratorAdapter<T* const, RadixTree<void*>::ConstIterator>         ConstIterator;

    /** Default constructor, does nothing */
    inline RadixTree()
      : m_VoidRadixTree(){}
    /** Copy-constructor
     *\param[in] x reference object */
    inline RadixTree(const RadixTree &x)
      : m_VoidRadixTree(x.m_VoidRadixTree){}
    /** Destructor, deallocates memory */
    inline ~RadixTree()
      {}

    /** Assignment operator
     *\param[in] x the object that should be copied */
    inline RadixTree &operator = (const RadixTree &x)
    {
      m_VoidRadixTree = x.m_VoidRadixTree;
      return *this;
    }

    /** Get the number of elements in the RadixTree */
    inline size_t count() const
    {
      return m_VoidRadixTree.count();
    }

    inline void insert(String key, T *value)
    {
      m_VoidRadixTree.insert(key, reinterpret_cast<void*>(const_cast<typename nonconst_type<T>::type*>(value)));
    }
    inline T *lookup(String key)
    {
      return reinterpret_cast<T*>(m_VoidRadixTree.lookup(key));
    }
    inline void remove(String key)
    {
      m_VoidRadixTree.remove(key);
    }

    /** Get an iterator pointing to the beginning of the RadixTree
     *\return iterator pointing to the beginning of the RadixTree */
    inline Iterator begin()
    {
      return Iterator(m_VoidRadixTree.begin());
    }
    /** Get a constant iterator pointing to the beginning of the RadixTree
     *\return constant iterator pointing to the beginning of the RadixTree */
    inline ConstIterator begin() const
    {
      return ConstIterator(m_VoidRadixTree.begin());
    }
    /** Get an iterator pointing to the end of the RadixTree + 1
     *\return iterator pointing to the end of the RadixTree + 1 */
    inline Iterator end()
    {
      return Iterator(m_VoidRadixTree.end());
    }
    /** Get a constant iterator pointing to the end of the RadixTree + 1
     *\return constant iterator pointing to the end of the RadixTree + 1 */
    inline ConstIterator end() const
    {
      return ConstIterator(m_VoidRadixTree.end());
    }

    /** Remove all elements from the RadixTree */
    inline void clear()
    {
      m_VoidRadixTree.clear();
    }

  private:
    /** The actual container */
    RadixTree<void*> m_VoidRadixTree;
};

/** @} */

#endif

