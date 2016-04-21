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

#ifndef KERNEL_UTILITIES_HASHTABLE_H
#define KERNEL_UTILITIES_HASHTABLE_H

#include <processor/types.h>
#include <utilities/lib.h>

/** @addtogroup kernelutilities
 * @{ */

/**
 * Hash table class.
 *
 * Handles hash collisions by chaining keys.
 *
 * The table is lazily created, so if a lookup or insertion is never
 * done, the instance will not consume any additional memory.
 *
 * The key type 'K' should have a method hash() which returns a
 * size_t hash that can be used to index into the bucket array.
 *
 * The key type 'K' should also be able to compare against other 'K'
 * types for equality.
 */
template<class K, class V>
class HashTable
{
    public:
        /**
         * To determine how many buckets you need, simply identify the
         * upper bound of the output of your hash function.
         *
         * Hashes that fall outside this range will be silently ignored.
         */
        HashTable(size_t numbuckets = 0) {
            m_Buckets = 0;
            m_nBuckets = 0;
            m_MaxBucket = numbuckets;
        }

        /**
         * Allows for late configuration of the hash table size, for
         * cases where the table size can be optimised.
         */
        void initialise(size_t numbuckets, bool preallocate = false) {
            if(m_Buckets) {
                return;
            }
            m_MaxBucket = numbuckets;
            m_nBuckets = 0;

            if(preallocate) {
                m_nBuckets = m_MaxBucket;
                m_Buckets = new bucket[m_nBuckets];
            }
        }

        virtual ~HashTable() {
            if(m_Buckets) {
                for (size_t i = 0; i < m_nBuckets; ++i)
                {
                    bucket *b = m_Buckets[i].next;
                    while (b)
                    {
                        bucket *d = b;
                        b = b->next;
                        delete d;
                    }
                }

                delete [] m_Buckets;
            }
        }

        /**
         * Do a lookup of the given key, and return either the value,
         * or NULL if the key is not in the hashtable.
         *
         * O(1) in the average case, with a hash function that rarely
         * collides.
         */
        V *lookup(K &k) const {
            // No buckets yet?
            if(!m_Buckets) {
                return 0;
            }

            size_t hash = k.hash();
            if(hash > m_MaxBucket) {
                return 0;
            }

            if (hash >= m_nBuckets)
            {
                // Not present.
                return 0;
            }

            bucket *b = &m_Buckets[hash];
            if(!b->set) {
                return 0;
            }

            if (b->key != k)
            {
                // Search the chain.
                bucket *chain = b->next;
                while (chain)
                {
                    if (chain->key == k)
                    {
                        b = chain;
                        break;
                    }

                    chain = chain->next;
                }

                if(!chain) {
                    return 0;
                }
            }

            return b->value;
        }

        /**
         * Insert the given value with the given key.
         */
        void insert(K &k, V *v) {
            // If this exact key already exists, we have more than
            // just a hash collision, and we must fail.
            if(lookup(k) != 0) {
                return;
            }

            if (!m_MaxBucket) {
                return;
            }

            size_t hash = k.hash();
            if(hash > m_MaxBucket) {
                return;
            }

            // Lazily create buckets if needed.
            if((hash + 1) > m_nBuckets) {
                size_t nextStep = max(max(m_nBuckets * 2, hash + 1), m_MaxBucket);
                bucket *newBuckets = new bucket[nextStep];
                if (m_Buckets) {
                    pedigree_std::copy(newBuckets, m_Buckets, m_nBuckets);
                    delete [] m_Buckets;
                }
                m_nBuckets = nextStep;
                m_Buckets = newBuckets;
            }

            // Do we need to chain?
            if (m_Buckets[hash].set)
            {
                // Yes.
                bucket *newb = new bucket;
                newb->key = k;
                newb->value = v;
                newb->set = true;
                newb->next = 0;

                bucket *last = &m_Buckets[hash];
                bucket *bucket = &m_Buckets[hash];
                while (bucket->next)
                {
                    bucket = bucket->next;
                }

                bucket->next = newb;
            }
            else
            {
                m_Buckets[hash].set = true;
                m_Buckets[hash].key = k;
                m_Buckets[hash].value = v;
                m_Buckets[hash].next = 0;
            }
        }

        /**
         * Remove the given key.
         */
        void remove(K &k) {
            if(lookup(k) == 0) {
                return;
            }

            if(!m_Buckets) {
                return;
            }

            size_t hash = k.hash();
            if(hash > m_MaxBucket) {
                return;
            }

            if(hash > m_nBuckets) {
                return;
            }

            bucket *b = &m_Buckets[hash];
            if(b->key == k) {
                if (b->next)
                {
                    // Carry the next entry into this position.
                    bucket *next = b->next;
                    m_Buckets[hash] = *next;
                    delete next;
                }
                else
                {
                    // This entry is available, no chain present.
                    m_Buckets[hash].set = false;
                }
            } else {
                // There's a chain, so we need to find the correct key in it.
                bucket *p = b;
                bucket *l = p;
                while (p)
                {
                    if (p->key == k)
                    {
                        l->next = p->next;
                        delete p;
                        break;
                    }

                    p = p->next;
                }
            }
        }

    private:
        struct bucket {
            bucket() : key(), value(0), next(0), set(false)
            {
            }

            K key;
            V *value;

            // Where hash collisions occur, we chain another value
            // to the original bucket.
            bucket *next;

            bool set;
        };

        bucket *m_Buckets;
        size_t m_nBuckets;
        size_t m_MaxBucket;
};

/** @} */

#endif

