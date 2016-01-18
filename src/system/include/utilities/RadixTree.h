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

#ifndef KERNEL_UTILITIES_RADIX_TREE_H
#define KERNEL_UTILITIES_RADIX_TREE_H

#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/Iterator.h>
#include <utilities/IteratorAdapter.h>
#include <Log.h>

/**\file  RadixTree.h
 *\author James Molloy <jamesm@osdev.org>
 *\date   Fri May  8 10:50:45 2009
 *\brief  Implements a Radix Tree, a kind of Trie with compressed keys. */

/** @addtogroup kernelutilities
 * @{ */

/** Dictionary class, aka Map for string keys. This is
 *  implemented as a Radix Tree - also known as a Patricia Trie.
 * \brief A key/value dictionary for string keys. */
template<class T>
class RadixTree
{
private:
    /** Tree node. */
    class Node
    {
    public:
        typedef Vector<Node *> childlist_t;
        enum MatchType
        {
            ExactMatch,   ///< Key matched node key exactly.
            NoMatch,      ///< Key didn't match node key at all.
            PartialMatch, ///< A subset of key matched the node key.
            OverMatch     ///< Key matched node key, and had extra characters.
        };

        Node(bool bCaseSensitive) :
            m_Key(),value(T()),m_Children(),m_pParent(0),
            m_bCaseSensitive(bCaseSensitive)
        {
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
        Node *findChild(const char *cpKey);

        /** Adds a new child. */
        void addChild(Node *pNode);

        /** Replaces a currently existing child. */
        void replaceChild(Node *pNodeOld, Node *pNodeNew);

        /** Removes a child (doesn't delete it) */
        void removeChild(Node *pChild);

        /** Compares cpKey and this node's key, returning the type of match
            found. */
        MatchType matchKey(const char *cpKey);

        /** Returns the first found child of the node. */
        Node *getFirstChild();

        /** Sets the node's key to the concatenation of \p cpKey and the
         *  current key.
         *\param cpKey Key to prepend to the current key. */
        void prependKey(const char *cpKey);

        void setKey(const char *cpKey);
        inline const char *getKey() {return m_Key;}
        inline void setValue(T pV) {value = pV;}
        inline T getValue() {return value;}
        inline void setParent(Node *pP) {m_pParent = pP;}
        inline Node *getParent() {return m_pParent;}

        /** Node key, zero terminated. */
        String m_Key;
        /** Node value.
            \note Parting from coding standard because Iterator requires the
                  member be called 'value'. */
        T value;
        /** Array of 16 pointers to 16 nodes (256 total). */
        childlist_t m_Children;
        /** Parent node. */
        Node *m_pParent;
        /** Controls case-sensitive matching. */
        bool m_bCaseSensitive;

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
    typedef ::Iterator<T, Node>       Iterator;
    /** Type of the constant bidirectional iterator */
    typedef typename Iterator::Const  ConstIterator;

    /** The default constructor, does nothing */
    RadixTree();
    /** The copy-constructor
     *\param[in] x the reference object to copy */
    RadixTree(const RadixTree<T> &x);
    /** Constructor that offers case sensitivity adjustment. */
    RadixTree(bool bCaseSensitive);
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
    void insert(String key, T value);
    /** Attempts to find an element with the given key.
     *\return the element found, or NULL if not found. */
    T lookup(String key) const;
    /** Attempts to remove an element with the given key. */
    void remove(String key);

    /** Clear the tree. */
    void clear();

    /** Erase one Element */
    Iterator erase(Iterator iter)
    {
        Node *iterNode = iter.__getNode();
        Node *next = iterNode->next();
        remove(String(iterNode->getKey()));
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
    size_t m_nItems;
    /** The tree's root. */
    Node *m_pRoot;
    /** Whether matches are case-sensitive or not. */
    bool m_bCaseSensitive;
};

template<class T>
RadixTree<T>::RadixTree() :
    m_nItems(0), m_pRoot(0), m_bCaseSensitive(true)
{
}

template<class T>
RadixTree<T>::RadixTree(bool bCaseSensitive) :
    m_nItems(0), m_pRoot(0), m_bCaseSensitive(bCaseSensitive)
{
}

template<class T>
RadixTree<T>::~RadixTree()
{
    clear();
    delete m_pRoot;
}

template<class T>
RadixTree<T>::RadixTree(const RadixTree &x) :
    m_nItems(0), m_pRoot(0), m_bCaseSensitive(x.m_bCaseSensitive)
{
    clear();
    delete m_pRoot;
    m_pRoot = cloneNode (x.m_pRoot, 0);
    m_nItems = x.m_nItems;
}

template<class T>
RadixTree<T> &RadixTree<T>::operator =(const RadixTree &x)
{
    clear();
    delete m_pRoot;
    m_pRoot = cloneNode (x.m_pRoot, 0);
    m_nItems = x.m_nItems;
    m_bCaseSensitive = x.m_bCaseSensitive;
    return *this;
}

template<class T>
size_t RadixTree<T>::count() const
{
    return m_nItems;
}

template<class T>
void RadixTree<T>::insert(String key, T value)
{
    if (!m_pRoot)
    {
        // The root node always exists and is a lambda transition node (zero-length
        // key). This removes the need for most special cases.
        m_pRoot = new Node(m_bCaseSensitive);
        m_pRoot->setKey(0);
    }

    Node *pNode = m_pRoot;

    const char *cpKey = static_cast<const char*>(key);

    while (true)
    {
        switch (pNode->matchKey(cpKey))
        {
            case Node::ExactMatch:
            {
                /// \todo should only increment m_nItems if this is the first
                ///       time the value has been set here.
                pNode->setValue(value);
                m_nItems++;
                return;
            }
            case Node::NoMatch:
            {
                FATAL("RadixTree: algorithmic error!");
                break;
            }
            case Node::PartialMatch:
            {
                // We need to create an intermediate node that contains the 
                // partial match, then adjust the key of this node.

                // Find the common key prefix.
                size_t i = 0;
                if (m_bCaseSensitive)
                    while (cpKey[i] == pNode->getKey()[i])
                        i++;
                else
                    while (toLower(cpKey[i]) == toLower(pNode->getKey()[i]))
                        i++;

                Node *pInter = new Node(m_bCaseSensitive);
                
                // Intermediate node's key is the common prefix of both keys.
                pInter->m_Key.assign(cpKey, i);

                // Must do this before pNode's key is changed.
                pNode->getParent()->replaceChild(pNode, pInter);

                // pNode's new key is the uncommon postfix.
                size_t len = pNode->m_Key.length();

                // Note: this is guaranteed to not require an allocation,
                // because it's smaller than the current string in m_Key. We'll
                // not overwrite because we're copying from deeper in the
                // string. The null write will suffice.
                pNode->m_Key.assign(&pNode->getKey()[i], len - i);

                // If the uncommon postfix of the key is non-zero length, we have
                // to create another node, a child of pInter.
                if (cpKey[i] != 0)
                {
                    Node *pChild = new Node(m_bCaseSensitive);
                    pChild->setKey(&cpKey[i]);
                    pChild->setValue(value);
                    pChild->setParent(pInter);
                    pInter->addChild(pChild);
                }
                else
                    pInter->setValue(value);

                pInter->setParent(pNode->getParent());
                pInter->addChild(pNode);
                pNode->setParent(pInter);

                m_nItems ++;
                return;
            }
            case Node::OverMatch:
            {
                cpKey += pNode->m_Key.length();

                Node *pChild = pNode->findChild(cpKey);
                if (pChild)
                {
                    pNode = pChild;
                    // Iterative case.
                    break;
                }
                else
                {
                    // No child - create a new one.
                    pChild = new Node(m_bCaseSensitive);
                    pChild->setKey(cpKey);
                    pChild->setValue(value);
                    pChild->setParent(pNode);
                    pNode->addChild(pChild);

                    m_nItems ++;
                    return;
                }
            }
        }
    }
}

template<class T>
T RadixTree<T>::lookup(String key) const
{
    if (!m_pRoot)
    {
        return T();
    }

    Node *pNode = m_pRoot;

    const char *cpKey = static_cast<const char*>(key);

    while (true)
    {
        switch (pNode->matchKey(cpKey))
        {
            case Node::ExactMatch:
                return pNode->getValue();
            case Node::NoMatch:
            case Node::PartialMatch:
                return T();
            case Node::OverMatch:
            {
                cpKey += pNode->m_Key.length();

                Node *pChild = pNode->findChild(cpKey);
                if (pChild)
                {
                    pNode = pChild;
                    // Iterative case.
                    break;
                }
                else
                    return T();
            }
        }
    }
}

template<class T>
void RadixTree<T>::remove(String key)
{
    if (!m_pRoot)
    {
        // The root node always exists and is a lambda transition node (zero-length
        // key). This removes the need for most special cases.
        m_pRoot = new Node(m_bCaseSensitive);
        m_pRoot->setKey(0);
    }

    Node *pNode = m_pRoot;

    const char *cpKey = static_cast<const char*>(key);

    // Our invariant is that the root node always exists. Therefore we must
    // special case here so it doesn't get deleted.
    if (*cpKey == 0)
    {
        m_pRoot->setValue(0);
        return;
    }

    while (true)
    {
        switch (pNode->matchKey(cpKey))
        {
            case Node::ExactMatch:
            {
                // Delete this node. If we set the value to zero, it is effectively removed from the map.
                // There are only certain cases in which we can delete the node completely, however.
                pNode->setValue(0);
                m_nItems --;

                // We have the invariant that the tree is always optimised. This means that when we delete a node
                // we only have to optimise the local branch. There are two situations that need covering:
                //   (a) No children. This means it's a leaf node and can be deleted. We then need to consider its parent,
                //       which, if its value is zero and now has zero or one children can be optimised itself.
                //   (b) One child. This is a linear progression and the child node's key can be changed to be concatenation of
                //       pNode's and its. This doesn't affect anything farther up the tree and so no recursion is needed.

                Node *pParent = 0;
                if (pNode->m_Children.count() == 0)
                {
                    // Leaf node, can just delete.
                    pParent = pNode->getParent();
                    pParent->removeChild(pNode);
                    delete pNode;

                    pNode = pParent;
                    // Optimise up the tree.
                    while (true)
                    {
                        if (pNode == m_pRoot) return;

                        if (pNode->m_Children.count() == 1 && pNode->getValue() == 0)
                            // Break out of this loop and get caught in the next
                            // if(pNode->m_nChildren == 1)
                            break;

                        if (pNode->m_Children.count() == 0 && pNode->getValue() == 0)
                        {
                            // Leaf node, can just delete.
                            pParent = pNode->getParent();
                            pParent->removeChild(pNode);
                            delete pNode;

                            pNode = pParent;
                            continue;
                        }
                        return;
                    }
                }

                if (pNode->m_Children.count() == 1)
                {
                    // Change the child's key to be the concatenation of ours and 
                    // its.
                    Node *pChild = pNode->getFirstChild();
                    pParent = pNode->getParent();

                    // Must call this before delete, so pChild doesn't get deleted.
                    pNode->removeChild(pChild);

                    pChild->prependKey(pNode->getKey());
                    pChild->setParent(pParent);
                    pParent->removeChild(pNode);
                    pParent->addChild(pChild);

                    delete pNode;
                }
                return;
            }
            case Node::NoMatch:
            case Node::PartialMatch:
                // Can't happen unless the key didn't actually exist.
                return;
            case Node::OverMatch:
            {
                cpKey += pNode->m_Key.length();

                Node *pChild = pNode->findChild(cpKey);
                if (pChild)
                {
                    pNode = pChild;
                    // Iterative case.
                    break;
                }
                else
                    return;
            }
        }
    }
}

template<class T>
typename RadixTree<T>::Node *RadixTree<T>::cloneNode(Node *pNode, Node *pParent)
{
    // Deal with the easy case first.
    if (!pNode)
        return 0;

    Node *n = new Node(m_bCaseSensitive);
    n->setKey(pNode->m_Key);
    n->setValue(pNode->value);
    n->setParent(pParent);

    for(typename RadixTree<T>::Node::childlist_t::Iterator it = pNode->m_Children.begin();
        it != pNode->m_Children.end();
        ++it)
    {
        n->addChild(cloneNode((*it), pParent));
    }

    return n;
}

template<class T>
void RadixTree<T>::clear()
{
    delete m_pRoot;
    m_pRoot = new Node(m_bCaseSensitive);
    m_pRoot->setKey(0);
    m_nItems = 0;
}

//
// RadixTree::Node implementation.
//

template<class T>
RadixTree<T>::Node::~Node()
{
    for(size_t n = 0; n < m_Children.count(); ++n)
    {
        delete m_Children[n];
    }
}

template<class T>
typename RadixTree<T>::Node *RadixTree<T>::Node::findChild(const char *cpKey)
{
    for(size_t n = 0; n < m_Children.count(); ++n)
    {
        if(m_Children[n]->matchKey(cpKey) != NoMatch)
            return m_Children[n];
    }
    return 0;
}

template<class T>
void RadixTree<T>::Node::addChild(Node *pNode)
{
    if(pNode)
        m_Children.pushBack(pNode);
}

template<class T>
void RadixTree<T>::Node::replaceChild(Node *pNodeOld, Node *pNodeNew)
{
    for(size_t n = 0; n < m_Children.count(); ++n)
    {
        if(m_Children[n] == pNodeOld)
        {
            m_Children.setAt(n, pNodeNew);
        }
    }
}

template<class T>
void RadixTree<T>::Node::removeChild(Node *pChild)
{
    for(typename RadixTree<T>::Node::childlist_t::Iterator it = m_Children.begin();
        it != m_Children.end();
        )
    {
        if((*it) == pChild)
        {
            it = m_Children.erase(it);
        }
        else
            ++it;
    }
}

template<class T>
typename RadixTree<T>::Node::MatchType RadixTree<T>::Node::matchKey(const char *cpKey)
{
    if (!m_Key.length()) return OverMatch;

    size_t i = 0;
    while (cpKey[i] && getKey()[i])
    {
        bool bMatch = false;
        if (m_bCaseSensitive)
        {
            bMatch = cpKey[i] == getKey()[i];
        }
        else
        {
            bMatch = toLower(cpKey[i]) == toLower(getKey()[i]);
        }

        if (!bMatch)
            return (i==0) ? NoMatch : PartialMatch;
        i++;
    }

    // Why did the loop exit?
    if (cpKey[i] == 0 && getKey()[i] == 0)
        return ExactMatch;
    else if (cpKey[i] == 0)
        return PartialMatch;
    else
        return OverMatch;
}

template<class T>
void RadixTree<T>::Node::setKey(const char *cpKey)
{
    m_Key.assign(cpKey);
}

template<class T>
typename RadixTree<T>::Node *RadixTree<T>::Node::getFirstChild()
{
    if(m_Children.count())
        return m_Children[0];

    return 0;
}

template<class T>
void RadixTree<T>::Node::prependKey(const char *cpKey)
{
    String temp = m_Key;
    m_Key.assign(cpKey);
    m_Key += temp;
}

template<class T>
typename RadixTree<T>::Node *RadixTree<T>::Node::doNext()
{
    Node *pNode = this;
    while ( (pNode == this) || (pNode && (!pNode->value)) )
    {
        Node *tmp;
        if (pNode->m_Children.count())
            pNode = pNode->getFirstChild();
        else
        {
            tmp = pNode;
            pNode = 0;
            while (tmp && tmp->m_pParent != 0 /* Root node */)
            {
                if ( (pNode=tmp->getNextSibling()) != 0)
                    break;
                tmp = tmp->m_pParent;
            }
            if (tmp->m_pParent == 0) return 0;
        }
    }
    return pNode;
}

template<class T>
typename RadixTree<T>::Node *RadixTree<T>::Node::getNextSibling()
{
    if (!m_pParent) return 0;

    bool b = false;
    for(typename RadixTree<T>::Node::childlist_t::Iterator it = m_pParent->m_Children.begin();
        it != m_pParent->m_Children.end();
        ++it)
    {
        if(b)
            return (*it);
        if((*it) == this)
            b = true;
    }

    return 0;
}

// Explicitly instantiate RadixTree<void*> early.
template class RadixTree<void*>;

/** @} */

#endif
