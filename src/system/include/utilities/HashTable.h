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
                m_Buckets = new bucket*[m_nBuckets];
                ByteSet(m_Buckets, 0, sizeof(bucket*) * m_nBuckets);
            }
        }

        virtual ~HashTable() {
            if(m_Buckets) {
                for (size_t i = 0; i < m_nBuckets; ++i)
                {
                    bucket *b = m_Buckets[i];
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

            bucket *b = m_Buckets[hash];
            if(!b) {
                return 0;
            }

            while(b && (b->key != k)) {
                b = b->next;
            }

            if(!b) {
                return 0;
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
                bucket **newBuckets = new bucket*[hash + 1];
                if (m_Buckets) {
                    MemoryCopy(newBuckets, m_Buckets, m_nBuckets * sizeof(bucket *));
                    delete [] m_Buckets;
                }
                ByteSet(&newBuckets[m_nBuckets], 0, (hash + 1 - m_nBuckets) * sizeof(bucket *));
                m_nBuckets = hash + 1;
                m_Buckets = newBuckets;
            }

            bucket *newb = new bucket;
            newb->key = k;
            newb->value = v;
            newb->next = 0;

            bucket *b = m_Buckets[hash];
            if(!b) {
                m_Buckets[hash] = newb;
            } else {
                // Add to existing chain.
                while(b->next) {
                    b = b->next;
                }

                b->next = newb;
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

            bucket *b = m_Buckets[hash];
            if(b->key == k) {
                m_Buckets[hash] = b->next;
            } else {
                bucket *p = b;
                while(b && (b->key != k)) {
                    p = b;
                    b = b->next;
                }

                if(b) {
                    p->next = b->next;
                }
            }

            delete b;
        }

    private:
        struct bucket {
            K key;
            V *value;

            // Where hash collisions occur, we chain another value
            // to the original bucket.
            bucket *next;
        };

        bucket **m_Buckets;
        size_t m_nBuckets;
        size_t m_MaxBucket;
};

/** @} */

#endif

