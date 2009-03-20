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

#ifndef KERNEL_UTILITIES_TREE_H
#define KERNEL_UTILITIES_TREE_H

#include <processor/types.h>
#include <utilities/Iterator.h>

/** @addtogroup kernelutilities
 * @{ */

/** Dictionary class, aka Map. This is implemented as an AVL self-balancing binary search
 *  tree.
 * \brief A key/value dictionary. */
template<class K, class E>
class Tree;

/** Tree specialisation for void* */
template<>
class Tree<void*,void*>
{
  private:
    /** Tree node. */
    struct Node
    {
      void *key;
      void *element;
      struct Node *leftChild;
      struct Node *rightChild;
      struct Node *parent;
      size_t height;
    };

  public:
    
    /** Random access iterator for the Tree. */
    /*class Iterator
    {
      public:
        Iterator() :
          pNode(0), pPreviousNode(0)
        {};
        Iterator(Iterator& it) :
          pNode(it.pNode), pPreviousNode(it.pPreviousNode)
        {};
        Iterator(Node* p) :
          pNode(p), pPreviousNode((p != 0) ? p->parent : 0)
        {};
        virtual ~Iterator()
        {};
        
        void* key()
        {
          if(pNode)
            return pNode->key;
          else
            return 0;
        }
        
        void* value()
        {
          if(pNode)
            return pNode->element;
          else
            return 0;
        }
        
        void operator ++ ()
        {
          traverseNext();
        }
        
        void* operator * ()
        {
          // pNode will be null when we reach the end of the list
          return reinterpret_cast<void*>(pNode);
        }
        
        bool operator != (Iterator& it)
        {
          return pNode != it.pNode;
        }
        
        bool operator == (Iterator& it)
        {
          return pNode == it.pNode;
        }
        
        Iterator& operator = (Iterator& it)
        {
          pNode = it.pNode;
          pPreviousNode = it.pPreviousNode;
          
          return *(const_cast<Iterator*>(this));
        }
        
      protected:
        Node* pNode;
        Node* pPreviousNode;
        
        void traverseNext()
        {
          if(pNode == 0)
            return;
          
          if((pPreviousNode == pNode->parent) && pNode->leftChild)
          {
            pPreviousNode = pNode;
            pNode = pNode->leftChild;
            traverseNext();
          }
          else if((pPreviousNode == pNode->leftChild) && !pNode->leftChild)
          {
            pPreviousNode = pNode;
            return;
          }
          else if((pPreviousNode == pNode) && pNode->rightChild)
          {
            pPreviousNode = pNode;
            pNode = pNode->rightChild;
            traverseNext();
          }
          else
          {
            pPreviousNode = pNode;
            pNode = pNode->parent;
            traverseNext();
          }
        }
    };*/

    /// \todo This actually will mean for each Tree you can only use one iterator at a time, which may
    ///       not be effective depending on how this is used.
    class IteratorNode
    {
      public:
        IteratorNode() : value(0), pNode(0), pPreviousNode(0)
        {};
        IteratorNode(Node* node, Node* prev) : value(node), pNode(node), pPreviousNode(prev)
        {
          // skip the root node, get to the lowest node in the tree
          traverseNext();
          value = pNode;
        };
        
        IteratorNode *next()
        {
          traverseNext();
          
          value = pNode;
          
          return this;
        }
        IteratorNode *previous()
        {
          return 0;
        }
        
        Node* value;
      
      private:
        
        Node* pNode;
        Node* pPreviousNode;
        
        void traverseNext()
        {
          if(pNode == 0)
            return;
          
          if((pPreviousNode == pNode->parent) && pNode->leftChild)
          {
            pPreviousNode = pNode;
            pNode = pNode->leftChild;
            traverseNext();
          }
          else if((((pNode->leftChild) && (pPreviousNode == pNode->leftChild)) || ((!pNode->leftChild) && (pPreviousNode != pNode))) && (pPreviousNode != pNode->rightChild))
          {
            pPreviousNode = pNode;
          }
          else if((pPreviousNode == pNode) && pNode->rightChild)
          {
            pPreviousNode = pNode;
            pNode = pNode->rightChild;
            traverseNext();
          }
          else
          {
            pPreviousNode = pNode;
            pNode = pNode->parent;
            traverseNext();
          }
        }
    };
    
    //typedef void**        Iterator;
    
    typedef ::TreeIterator<void*, IteratorNode> Iterator;
    /** Constant random-access iterator for the Tree */
    typedef void* const*  ConstIterator;
    
    /** The default constructor, does nothing */
    Tree();
    /** The copy-constructor
     *\param[in] x the reference object to copy */
    Tree(const Tree &x);
    /** The destructor, deallocates memory */
    ~Tree();

    /** The assignment operator
     *\param[in] x the object that should be copied */
    Tree &operator = (const Tree &x);

    /** Get the number of elements in the Tree
     *\return the number of elements in the Tree */
    size_t count() const;
    /** Add an element to the Tree.
     *\param[in] key the key
     *\param[in] value the element */
    void insert(void *key, void *value);
    /** Attempts to find an element with the given key.
     *\return the element found, or NULL if not found. */
    void *lookup(void *key);
    /** Attempts to remove an element with the given key. */
    void remove(void *key);

    /** Clear the Vector */
    void clear();
    /** Erase one Element */
    Iterator erase(Iterator iter);

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    inline Iterator begin()
    {
      if(m_Begin)
        delete m_Begin;
      m_Begin = new IteratorNode(root, 0);
      return Iterator(m_Begin);
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    inline ConstIterator begin() const
    {
      return 0;
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    inline Iterator end()
    {
      return Iterator(0);
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    inline ConstIterator end() const
    {
      return 0;
    }

  private:
    void rotateLeft(Node *n);
    void rotateRight(Node *n);
    size_t height(Node *n);
    int balanceFactor(Node *n);
    void rebalanceNode(Node *n);

    void traverseNode_Insert(Node *n);
    void traverseNode_Remove(Node *n);
    
    Node *root;
    size_t nItems;
    
    IteratorNode* m_Begin;
};

/** Tree template specialisation for key pointers and element pointers. Just forwards to the
 * void* template specialisation of Tree.
 *\brief A vector / dynamic array for pointer types */
template<class K, class V>
class Tree<K*,V*>
{
  public:
    /** Random-access iterator for the Vector */
    typedef Tree<void*,void*>::Iterator      Iterator;
    /** Contant random-access iterator for the Vector */
    typedef Tree<void*,void*>::ConstIterator ConstIterator;

    /** The default constructor, does nothing */
    inline Tree()
      : m_VoidTree(){};
    /** The copy-constructor
     *\param[in] x the reference object */
    inline Tree(const Tree &x)
      : m_VoidTree(x.m_VoidTree){}
    /** The destructor, deallocates memory */
    inline ~Tree()
    {}

    /** The assignment operator
     *\param[in] x the object that should be copied */
    inline Tree &operator = (const Tree &x)
      {m_VoidTree = x.m_VoidTree;return *this;}

    /** Get the number of elements in the Tree
     *\return the number of elements in the Tree */
    inline size_t count() const
      {return m_VoidTree.count();}
    /** Add an element to the Tree
     *\param[in] value the element */
    inline void insert(K *key, V *value)
      {m_VoidTree.insert(reinterpret_cast<void*>(key),reinterpret_cast<void*>(value));}
    /** Attempts to find an element with the given key.
     *\return the removed element */
    inline V *lookup(K *key)
      {return reinterpret_cast<V*>(m_VoidTree.lookup(reinterpret_cast<void*>(key)));}
    /** Attempts to find an element with the given key, then remove it. */
    inline void remove(K *key)
      {return m_VoidTree.remove(reinterpret_cast<void*>(key));}

    /** Clear the Tree */
    inline void clear()
      {m_VoidTree.clear();}
    /** Erase one Element */
    Iterator erase(Iterator iter)
      {return reinterpret_cast<Iterator>(m_VoidTree.erase(reinterpret_cast<typename Tree<void*,void*>::Iterator>(iter)));}

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    inline Iterator begin()
    {
      return reinterpret_cast<Iterator>(m_VoidTree.begin());
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    inline ConstIterator begin() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidTree.begin());
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    inline Iterator end()
    {
      return reinterpret_cast<Iterator>(m_VoidTree.end());
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    inline ConstIterator end() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidTree.end());
    }

  private:
    /** The actual container */
    Tree<void*,void*> m_VoidTree;
};

/** Tree template specialisation for integer keys and element pointers. Just forwards to the
 * void* template specialisation of Tree. */
template<class V>
class Tree<size_t,V*>
{
  public:
    /** Random access iterator for the Tree. */
    /*class Iterator : public Tree<void*,void*>::Iterator
    {
      public:
        size_t key()
        {
          if(pNode)
            return reinterpret_cast<size_t>(pNode->key);
          else
            return 0;
        }
    };*/
    
    /// \todo Custom key/value implementations
    typedef Tree<void*,void*>::Iterator      Iterator;
    /** Contant random-access iterator for the Vector */
    typedef Tree<void*,void*>::ConstIterator ConstIterator;

    /** The default constructor, does nothing */
    inline Tree()
      : m_VoidTree(){};
    /** The copy-constructor
     *\param[in] x the reference object */
    inline Tree(const Tree &x)
      : m_VoidTree(x.m_VoidTree){}
    /** The destructor, deallocates memory */
    inline ~Tree()
    {}

    /** The assignment operator
     *\param[in] x the object that should be copied */
    inline Tree &operator = (const Tree &x)
      {m_VoidTree = x.m_VoidTree;return *this;}

    /** Get the number of elements in the Tree
     *\return the number of elements in the Tree */
    inline size_t count() const
      {return m_VoidTree.count();}
    /** Add an element to the Tree
     *\param[in] value the element */
    inline void insert(size_t key, V *value)
      {m_VoidTree.insert(reinterpret_cast<void*>(key),reinterpret_cast<void*>(value));}
    /** Attempts to find an element with the given key.
     *\return the removed element */
    inline V *lookup(size_t key)
      {return reinterpret_cast<V*>(m_VoidTree.lookup(reinterpret_cast<void*>(key)));}
    /** Attempts to find an element with the given key, then remove it. */
    inline void remove(size_t key)
      {return m_VoidTree.remove(reinterpret_cast<void*>(key));}

    /** Clear the Tree */
    inline void clear()
      {m_VoidTree.clear();}
    /** Erase one Element */
    Iterator erase(Iterator iter)
      //{return *reinterpret_cast<Iterator*>(&(m_VoidTree.erase(*reinterpret_cast<Tree<void*,void*>::Iterator*>(&iter))));}
      {return reinterpret_cast<Iterator>(m_VoidTree.erase(reinterpret_cast<typename Tree<void*,void*>::Iterator>(iter)));}

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    inline Iterator begin()
    {
      //return *reinterpret_cast<Iterator*>(&(m_VoidTree.begin()));
      //return reinterpret_cast<Iterator>(m_VoidTree.begin());
      return m_VoidTree.begin();
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    inline ConstIterator begin() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidTree.begin());
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    inline Iterator end()
    {
      //return *reinterpret_cast<Iterator*>(&(m_VoidTree.end()));
      //return reinterpret_cast<Iterator>(m_VoidTree.end());
      return m_VoidTree.end();
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    inline ConstIterator end() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidTree.end());
    }

  private:
    /** The actual container */
    Tree<void*,void*> m_VoidTree;
};

/** Tree template specialisation for integer keys and integer elements. Just forwards to the
 * void* template specialisation of Tree. */
template<>
class Tree<size_t,size_t>
{
  public:
    /** Random access iterator for the Tree. */
    /*class Iterator : public Tree<void*,void*>::Iterator
    {
      public:
        size_t key()
        {
          if(pNode)
            return reinterpret_cast<size_t>(pNode->key);
          else
            return 0;
        }
        
        size_t value()
        {
          if(pNode)
            return reinterpret_cast<size_t>(pNode->element);
          else
            return 0;
        }
        
        size_t operator * ()
        {
          if(pNode)
            return reinterpret_cast<size_t>(pNode->element);
          else
            return 0;
        }
    };*/
    
    /// \todo Custom key/value implementations
    typedef Tree<void*,void*>::Iterator      Iterator;
    /** Contant random-access iterator for the Vector */
    typedef Tree<void*,void*>::ConstIterator ConstIterator;

    /** The default constructor, does nothing */
    inline Tree()
      : m_VoidTree(){};
    /** The copy-constructor
     *\param[in] x the reference object */
    inline Tree(const Tree &x)
      : m_VoidTree(x.m_VoidTree){}
    /** The destructor, deallocates memory */
    inline ~Tree()
    {}

    /** The assignment operator
     *\param[in] x the object that should be copied */
    inline Tree &operator = (const Tree &x)
      {m_VoidTree = x.m_VoidTree;return *this;}

    /** Get the number of elements in the Tree
     *\return the number of elements in the Tree */
    inline size_t count() const
      {return m_VoidTree.count();}
    /** Add an element to the Tree
     *\param[in] value the element */
    inline void insert(size_t key, size_t value)
      {m_VoidTree.insert(reinterpret_cast<void*>(key),reinterpret_cast<void*>(value));}
    /** Attempts to find an element with the given key.
     *\return the removed element */
    inline size_t lookup(size_t key)
      {return reinterpret_cast<size_t>(m_VoidTree.lookup(reinterpret_cast<void*>(key)));}
    /** Attempts to find an element with the given key, then remove it. */
    inline void remove(size_t key)
      {return m_VoidTree.remove(reinterpret_cast<void*>(key));}

    /** Clear the Tree */
    inline void clear()
      {m_VoidTree.clear();}
    /** Erase one Element */
    Iterator erase(Iterator iter)
      {return m_VoidTree.erase(iter);}
      //{return *reinterpret_cast<Iterator*>(&(m_VoidTree.erase(*reinterpret_cast<Tree<void*,void*>::Iterator*>(&iter))));}
      //{return reinterpret_cast<Iterator>(m_VoidTree.erase(reinterpret_cast< Tree<void*,void*>::Iterator>(iter)));}

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    inline Iterator begin()
    {
      //return *reinterpret_cast<Iterator*>(&(m_VoidTree.begin()));
      //return reinterpret_cast<Iterator>(m_VoidTree.begin());
      return m_VoidTree.begin();
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    inline ConstIterator begin() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidTree.begin());
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    inline Iterator end()
    {
      //return *reinterpret_cast<Iterator*>(&(m_VoidTree.end()));
      //return reinterpret_cast<Iterator>(m_VoidTree.end());
      return m_VoidTree.end();
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    inline ConstIterator end() const
    {
      return reinterpret_cast<ConstIterator>(m_VoidTree.end());
    }

  private:
    /** The actual container */
    Tree<void*,void*> m_VoidTree;
};

/** @} */

#endif
