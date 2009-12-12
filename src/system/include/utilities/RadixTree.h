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

/**\file  RadixTree.h
 *\author James Molloy <jamesm@osdev.org>
 *\date   Fri May  8 10:50:45 2009
 *\brief  Implements a Radix Tree, a kind of Trie with compressed keys. */

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
        struct NodePtr
        {Node *p[16];};

        enum MatchType
        {
            ExactMatch,   ///< Key matched node key exactly.
            NoMatch,      ///< Key didn't match node key at all.
            PartialMatch, ///< A subset of key matched the node key.
            OverMatch     ///< Key matched node key, and had extra characters.
        };

        Node() :
            m_pKey(0),value(0),m_pParent(0),m_nChildren(0)
        {
            memset(reinterpret_cast<uint8_t*>(m_pChildren), 0, 16*sizeof(NodePtr*));
        }

        ~Node ();

        /** Get the next data structure in the list
         *\return pointer to the next data structure in the list */
        Node *next()
        {
            return doNext();
        }
        /** Get the previous data structure in the list
         *\return pointer to the previous data structure in the list
         *\note Not implemented! */
        Node *previous()
        {return 0;}
        
        /** Locates a child of this node, given the key portion of key
            (lookahead on the first token) */
        Node *findChild(const uint8_t *cpKey);

        /** Adds a new child. */
        void addChild(Node *pNode);

        /** Replaces a currently existing child. */
        void replaceChild(Node *pNodeOld, Node *pNodeNew);

        /** Removes a child (doesn't delete it) */
        void removeChild(Node *pChild);

        /** Compares cpKey and this node's key, returning the type of match
            found. */
        MatchType matchKey(const uint8_t *cpKey);

        /** Returns the first found child of the node. */
        Node *getFirstChild();

        /** Sets the node's key to the concatenation of \p cpKey and the
         *  current key.
         *\param cpKey Key to prepend to the current key. */
        void prependKey(const uint8_t *cpKey);

        void setKey(const uint8_t *cpKey);
        inline uint8_t *getKey() {return m_pKey;}
        inline void setValue(void *pV) {value = pV;}
        inline void *getValue() {return value;}
        inline void setParent(Node *pP) {m_pParent = pP;}
        inline Node *getParent() {return m_pParent;}

        /** Node key, zero terminated. */
        uint8_t *m_pKey;
        /** Node value.
            \note Parting from coding standard because Iterator requires the
                  member be called 'value'. */
        void *value;
        /** Array of 16 pointers to 16 nodes (256 total). */
        NodePtr *m_pChildren[16];
        /** Parent node. */
        Node *m_pParent;
        /** Number of children. */
        size_t m_nChildren;

    private:
        Node(const Node&);
        Node &operator =(const Node&);
        
        /** Returns the next Node to look at during an in-order iteration. */
        Node *doNext();

        /** Returns the node's next sibling, by looking at its parent's children. */
        Node *getNextSibling();
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

    /** Clear the tree. */
    void clear();


    /** Erase one Element */
    Iterator erase(Iterator iter)
    {
        Node *iterNode = iter.__getNode();
        Node *next = iterNode->next();
        remove(String(reinterpret_cast<const char*>(iterNode->getKey())));
        Iterator ret(next);
        return ret;
    }

    /** Get an iterator pointing to the beginning of the List
     *\return iterator pointing to the beginning of the List */
    inline Iterator begin()
    {
        Iterator it(m_pRoot->next());
        return it;
    }
    /** Get a constant iterator pointing to the beginning of the List
     *\return constant iterator pointing to the beginning of the List */
    inline ConstIterator begin() const
    {
        ConstIterator it(m_pRoot->next());
        return it;
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
    /** Internal function to create a copy of a subtree. */
    Node *cloneNode(Node *node, Node *parent);

    /** Number of items in the tree. */
    int m_nItems;
    /** The tree's root. */
    Node *m_pRoot;
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
    Iterator erase(Iterator iter)
    {
        return m_VoidRadixTree.erase(iter.__getIterator());
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

