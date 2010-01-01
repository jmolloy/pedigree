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

#include <utilities/Trie.h>
#include <utilities/assert.h>
extern "C" void printf(const char *fmt, ...);
Trie<void*>::Trie() :
    m_pRoot(new Node(0)), m_nLongestPath(0), m_Lock(), m_RemovalCounter(0)
{
}

Trie<void*>::~Trie()
{
}

void Trie<void*>::insert(uint8_t *pKey, void *pValue)
{
    while (!m_Lock.enter())
        ;

    size_t len = strlen(reinterpret_cast<const char*>(pKey));

    Node *pNode, *pChild, *pTmpChild;
    size_t n = 0;
    pNode = m_pRoot;

    while (true)
    {
        if (n >= (len*2))
        {
            pNode->value = pValue;
            break;
        }

        // Grab the key index.
        uint8_t k = pKey[n>>1];
        if (n & 1)
            k >>= 4;
        else
            k &= 0x0F;

        // Loop until we either have no child or can lock the child.
        pTmpChild = 0;
        while(true)
        {
            // Get the child.
            pChild = const_cast<Node*> (pNode->children[k]);
            // If it exists and we can lock it, exit.
            if (pChild)
                break;

            // If it didn't exist, create a new child (if one hasn't already
            // been created).
            if (!pTmpChild)
            {
                pTmpChild = new Node((n == (len*2)) ? pValue : 0);
                pTmpChild->parent = pNode;
            }

            Atomic<volatile Node *> atom(pNode->children[k]);
            if(atom.compareAndSwap(0, pTmpChild))
            {
                pNode->children[k] = static_cast<volatile Node *>(atom);
                
                // CAS succeeded, ensure that pTmpChild is not deleted.
                pChild = pTmpChild;
                pTmpChild = 0;
                break;
            }
            else
                pNode->children[k] = static_cast<volatile Node *>(atom);
            
            /*
            if (__sync_bool_compare_and_swap(&pNode->children[k], 0, pTmpChild))
            {
                // CAS succeeded, ensure that pTmpChild is not deleted.
                pChild = pTmpChild;
                pTmpChild = 0;
                break;
            }
            */
            // CAS failed, drop through and repeat.
        }

        delete pTmpChild;

        // Recurse.
        pNode = pChild;
        n ++;
    }

    m_Lock.leave();
}

void *Trie<void*>::lookup(uint8_t *pKey)
{
    while (!m_Lock.enter())
        ;

    size_t len = strlen(reinterpret_cast<const char*>(pKey));

    Node *pNode, *pChild;
    size_t n = 0;
    pNode = m_pRoot;

    void *pRet = 0;

    while (true)
    {
        if (n >= (len*2))
        {
            pRet = pNode->value;
            break;
        }

        // Grab the key index.
        uint8_t k = pKey[n>>1];
        if (n & 1)
            k >>= 4;
        else
            k &= 0x0F;

        // Get the child.
        pChild = const_cast<Node*> (pNode->children[k]);
        if (!pChild)
            break;

        // Recurse.
        pNode = pChild;
        n ++;
    }

    m_Lock.leave();

    return pRet;
}

void Trie<void*>::remove(uint8_t *pKey)
{
    size_t len = strlen(reinterpret_cast<const char*>(pKey));

    Node *pNode, *pChild, *pTmpChild;
    size_t n = 0;
    pNode = m_pRoot;

    while (!m_Lock.enter())
        ;

    while (true)
    {
        if (n >= len*2)
        {
            pNode->value = 0;
            break;
        }

        // Grab the key index.
        uint8_t k = pKey[n>>1];
        if (n & 1)
            k >>= 4;
        else
            k &= 0x0F;

        // Get the child.
        pChild = const_cast<Node*> (pNode->children[k]);
        // If it doesn't exist or we can't lock it, exit.
        if (!pChild)
            break;

        // Recurse.
        pNode = pChild;
        n ++;
    }

    m_Lock.leave();

    size_t v = (m_RemovalCounter += 1);

    if ( (v % REMOVAL_RATE) == 0 && m_Lock.acquire() )
    {
        // Perform a depth-first search of the tree, looking for leaves.
        Node *pNode = m_pRoot;
        size_t lastIdx = 0;

        // Traverse directly to the bottom of the tree.
        while (true)
        {
            bool b = false;
            for (size_t k = 0; k < 16; k++)
                if (pNode->children[k])
                {
                    pNode = const_cast<Node*>(pNode->children[k]);
                    b = true;
                    break;
                }
            if (!b)
                break;
        }

        // Now pNode is the bottom-leftmost node in the tree, work our
        // way back to the root.
        while (pNode)
        {
            // Find this node's parent link.
            Node *pParent = pNode->parent;
            size_t k = 0;
            if (pParent)
            {
                for (k = 0; k < 16; k++)
                    if (pParent->children[k] == pNode)
                        break;
            }
            // Check leaf-itude.
            bool bIsLeaf = true;
            bool bHasMoreChildren = false;
            size_t i = 0;
            for (i = 0; i < 16; i++)
            {
                if (pNode->children[i])
                {
                    bIsLeaf = false;
                    if (i >= lastIdx)
                    {
                        bHasMoreChildren = true;
                        break;
                    }
                }
            }

            if (bIsLeaf && pNode->value == 0 && pParent != 0)
            {
                // Deletable.
                delete pNode;
                pParent->children[k] = 0;
            }

            if (!bHasMoreChildren)
            {
                lastIdx = k+1;
                pNode = pParent;
            }
            else
            {
                lastIdx = 0;
                pNode = const_cast<Node*>(pNode->children[i]);
            }
        }
        m_Lock.release();
    }
}

Trie<void*>::Iterator::Iterator(Trie<void*> *pParent) :
    m_pKey(0), m_nKeyLen(0), m_nMaxKeyLen(0), m_Value(0), m_pParent(pParent),
    m_bFinished(false)
{
    if (!pParent)
    {
        m_bFinished = true;
        return;
    }

    // Initialise by looking for the leftmost child.
    m_pKey = new uint8_t[/*pParent->m_nLongestPath*/256*2];
    m_nMaxKeyLen = /*pParent->m_nLongestPath*/256*2;

    m_Value = m_pParent->m_pRoot->value;

    while (!m_bFinished && m_Value == 0)
        next();
}

Trie<void*>::Iterator::~Iterator()
{
    delete [] m_pKey;
}

Trie<void*>::Iterator &Trie<void*>::Iterator::operator ++ ()
{
    m_Value = 0;
    while (!m_bFinished && m_Value == 0)
        next();
    return *this;
}

void Trie<void*>::Iterator::next()
{
    while (!m_pParent->m_Lock.enter())
        ;

    // Set the next 4 bits of the key to zero, and extend the key length.
    uint8_t k = m_pKey[m_nKeyLen>>1];
    if (m_nKeyLen & 1)
        k &= 0x0F;
    else
        k = 0;
    m_pKey[m_nKeyLen>>1] = k;
    m_nKeyLen++;

    Node *pNode = traverse(0, m_pParent->m_pRoot);
    if (!pNode)
        m_bFinished = true;

    m_pParent->m_Lock.leave();
}

/// \todo Remove the recursion in this algorithm.
///       -- Going to be difficult as it is essentially a depth-first search.
Trie<void*>::Node *Trie<void*>::Iterator::traverse(size_t n, Node *pNode)
{
    if (n >= m_nKeyLen)
    {
        m_Value = pNode->value;
        return pNode;
    }
    // Grab the key index.
    uint8_t k = m_pKey[n>>1];
    if (n & 1)
        k >>= 4;
    else
        k &= 0x0F;

    Node *pRet = 0;
    while (true)
    {
        if (k >= 16)
            return 0;

        // Grab the child, if it exists, and attempt to lock it.
        Node *pChild = const_cast<Node*> (pNode->children[k]);

        if (!pChild)
        {
            k ++;
            continue;
        }

        pRet = traverse(n+1, pChild);

        if (pRet) break;

        if (n & 1)
            m_pKey[n>>1] = m_pKey[n>>1] & 0x0F;
        else
            m_pKey[n>>1] = 0x00;
        m_nKeyLen = n+1;
        k ++;
    }
    assert(pRet);

    // Write back key.
    uint8_t k2 = m_pKey[n>>1];
    if (n & 1)
        k2 = (k2 & 0x0F) | (k << 4);
    else
        k2 = (k2 & 0xF0) | k;
    m_pKey[n>>1] = k2;

    return pRet;
}
