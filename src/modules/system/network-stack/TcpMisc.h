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

#ifndef MACHINE_TCPMISC_H
#define MACHINE_TCPMISC_H

#include <utilities/Tree.h>
#include <utilities/Iterator.h>
#include <process/Mutex.h>
#include <LockGuard.h>
#include "Endpoint.h"

#include <Log.h>

/** A TCP "Buffer" (also known as a stream) */
class TcpBuffer
{
  public:

    TcpBuffer() :
      m_Buffer(0), m_BufferSize(0), m_DataSize(0), m_Reader(0), m_Writer(0), m_Lock(false)
    {
      setSize(32768);
    };
    virtual ~TcpBuffer()
    {
      LockGuard<Mutex> guard(m_Lock);
      setSize(0);
    };

    /** Writes data to the buffer */
    size_t write(uintptr_t buffer, size_t nBytes);
    /** Reads data from the buffer */
    size_t read(uintptr_t buffer, size_t nBytes, bool bDoNotMove = false);

    /** Gets the number of bytes of data in the buffer */
    inline size_t getDataSize()
    {
        return m_DataSize;
    }
    /** Gets the size of the buffer */
    inline size_t getSize()
    {
      // Locked so the size given is a valid representation
        LockGuard<Mutex> guard(m_Lock);
        return m_BufferSize;
    }
    /** Sets the size of the buffer (ie, resize) */
    void setSize(size_t newBufferSize);

    /** Retrieves the number of bytes remaining in the buffer */
    inline size_t getRemainingSize()
    {
        return m_BufferSize - m_DataSize;
    }

  private:

    /** The actual buffer itself */
    uintptr_t m_Buffer;

    /** Current buffer size */
    size_t m_BufferSize;

    /** Data size */
    size_t m_DataSize;

    /** Reader offset */
    size_t m_Reader;

    /** Writer offset */
    size_t m_Writer;

    /** Buffer lock */
    Mutex m_Lock;
};

/** Connection state block handle */
struct StateBlockHandle
{
  StateBlockHandle() :
    localPort(0), remotePort(0), remoteHost(), listen(false)
  {};

  uint16_t localPort;
  uint16_t remotePort;
  Endpoint::RemoteEndpoint remoteHost;

  bool listen;

  bool operator == (StateBlockHandle a)
  {
    if(a.listen) // Require the client to want listen sockets only
    {
      if(listen)
        return (localPort == a.localPort);
    }
    else
    {
      // NOTICE_NOLOCK("Operator == [" << localPort << ", " << a.localPort << "] [" << remotePort << ", " << a.remotePort << "]" << " [" << remoteHost.ip.toString() << ", " << a.remoteHost.ip.toString() << "]");
      return ((localPort == a.localPort) && (remoteHost.ip == a.remoteHost.ip) && (remotePort == a.remotePort));
    }
    return false;
  }

  bool operator > (StateBlockHandle a)
  {
    return true;
    if(listen && a.listen)
      return (localPort > a.localPort);
    else
    {
      //NOTICE("Operator > : [" << localPort << ", " << a.localPort << "] [" << remotePort << ", " << a.remotePort << "]");
      if(localPort)
        return ((localPort > a.localPort) && (remotePort > a.remotePort));
      else
        return (remotePort > a.remotePort);
    }
  }
};

/** Special case for Tree with a state block handle as the key */
template <>
class Tree<StateBlockHandle,void*>
{
  private:

    /** Tree node. */
    struct Node
    {
      Node() :
        key(), element(0), leftChild(0), rightChild(0), parent(0), height(0)
      {};

      StateBlockHandle key;
      void *element;
      struct Node *leftChild;
      struct Node *rightChild;
      struct Node *parent;
      size_t height;
    };

  public:

    class IteratorNode
    {
      public:
        IteratorNode() : value(0), pNode(0), pPreviousNode(0)
        {};
        IteratorNode(Node* node, Node* prev) : value(node), pNode(node), pPreviousNode(prev)
        {};

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
    };

    typedef ::Iterator<void*, IteratorNode> Iterator;
    //typedef void**           Iterator;
    /** Contant random-access iterator for the Vector */
    typedef void** const*     ConstIterator;

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
    /** Add an element to the Tree
     *\param[in] value the element */
    void insert(StateBlockHandle key, void *value);
    /** Attempts to find an element with the given key.
     *\return the removed element */
    void *lookup(StateBlockHandle key);
    /** Attempts to find an element with the given key, then remove it. */
    void remove(StateBlockHandle key);

    /** Clear the Tree */
    void clear();
    /** Erase one Element */
    Iterator erase(Iterator iter);

    /** Get an iterator pointing to the beginning of the Vector
     *\return iterator pointing to the beginning of the Vector */
    Iterator begin()
    {
      return Iterator(m_Begin);
    }
    /** Get a constant iterator pointing to the beginning of the Vector
     *\return constant iterator pointing to the beginning of the Vector */
    ConstIterator begin() const
    {
      return 0;
    }
    /** Get an iterator pointing to the last element + 1
     *\return iterator pointing to the last element + 1 */
    Iterator end()
    {
      return Iterator(0);
    }
    /** Get a constant iterator pointing to the last element + 1
     *\return constant iterator pointing to the last element + 1 */
    ConstIterator end() const
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

    // root node
    Node *root;
    size_t nItems;

    IteratorNode* m_Begin;

    /** The actual container */
    //Tree<void*,void*> m_VoidTree;
};

/** Tree template specialisation for integer keys and element pointers. Just forwards to the
 * void* template specialisation of Tree. */
template<class V>
class Tree<StateBlockHandle,V*>
{
  public:
    /** Random-access iterator for the Vector */
    typedef Tree<StateBlockHandle,void*>::Iterator      Iterator;
    /** Contant random-access iterator for the Vector */
    typedef Tree<StateBlockHandle,void*>::ConstIterator ConstIterator;

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
    inline void insert(StateBlockHandle key, V *value)
      {m_VoidTree.insert(key,reinterpret_cast<void*>(value));}
    /** Attempts to find an element with the given key.
     *\return the removed element */
    inline V *lookup(StateBlockHandle key)
      {return reinterpret_cast<V*>(m_VoidTree.lookup(key));}
    /** Attempts to find an element with the given key, then remove it. */
    inline void remove(StateBlockHandle key)
      {return m_VoidTree.remove(key);}

    /** Clear the Tree */
    inline void clear()
      {m_VoidTree.clear();}
    /** Erase one Element */
    Iterator erase(Iterator iter)
      {return m_VoidTree.erase(iter);} //reinterpret_cast<Iterator>(m_VoidTree.erase(reinterpret_cast<typename Tree<void*,void*>::Iterator>(iter)));}

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
    Tree<StateBlockHandle,void*> m_VoidTree;
};

#endif
