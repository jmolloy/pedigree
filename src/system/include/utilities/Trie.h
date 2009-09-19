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
#ifndef TRIE_H
#define TRIE_H

#include <processor/types.h>
#include <utilities/assert.h>
#include <utilities/UnlikelyLock.h>

template<class T>
class Trie;

#define REMOVAL_RATE 1

/** A lock free 16-ary trie. */
template<>
class Trie<void*>
{
public:
    Trie();
    ~Trie();

    void insert(uint8_t *pKey, void *pValue);
    void *lookup(uint8_t *pKey);
    void remove(uint8_t *pKey);

private:
    /** Internal trie node. */
    struct Node
    {
        Node (void *v) : parent(0), value(v)
        {
            memset(children, 0, sizeof(Node*)*16);
        }
        Node          *parent;
        void          *value;
        volatile Node *children[16];
    };

public:
    class Iterator
    {
    public:
        Iterator(Trie<void*> *pParent);
        Iterator (const Iterator &other);
        ~Iterator();

        Iterator &operator ++ ();
        void *operator * ()
        {return m_Value;}
        void *operator -> ()
        {return m_Value;}

        const uint8_t *key()
        {
            // Ensure null-termination.
            assert((m_nKeyLen & 1) == 0);
            m_pKey[m_nKeyLen>>1] = 0x00;
            return const_cast<const uint8_t*>(m_pKey);
        }

        bool operator != (const Iterator &pOther)
        {
            return ! operator == (pOther);
        }

        bool operator == (const Iterator &pOther)
        {
            return (pOther.m_bFinished == m_bFinished);
        }

    private:
        Iterator &operator = (const Iterator &other);

        void next();
        Node *traverse(size_t n, Node *pNode);

        uint8_t       *m_pKey;
        size_t         m_nKeyLen;
        size_t         m_nMaxKeyLen;
        void          *m_Value;
        Trie          *m_pParent;
        bool           m_bFinished;
    };

    inline Iterator begin()
    {
        return Iterator(this);
    }
    inline Iterator end()
    {
        return Iterator(0);
    }

private:

    Trie(const Trie<void*> &other);
    Trie &operator = (const Trie<void*> &other);

    Node *m_pRoot;
    size_t m_nLongestPath;
    UnlikelyLock m_Lock;
    Atomic<size_t> m_RemovalCounter;
};

#endif
