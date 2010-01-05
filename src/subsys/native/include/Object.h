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

#ifndef _PEDIGREE_OBJECT_H
#define _PEDIGREE_OBJECT_H

#include <types.h>

/** Object is merely the base class for all native subsystem objects. */
class Object
{
    public:
        Object()
        {
        }
        virtual ~Object()
        {
        }

        /** \brief Serialises all relevant class data into a buffer.
         *
         * serialise is called when a request needs to be sent to the kernel
         * to perform an action. The class provides an implementation which
         * fills the buffer with class data that is deemed relevant. The
         * buffer is then transmitted to the kernel where it is passed to a
         * kernel-side object and unseralised.
         *
         * @param buffer must be filled with either 0 (serialisation failed)
         *               or the address of the serialised buffer
         * @param length must contain the length of the allocated buffer, in
         *               bytes
         */
        virtual void serialise(uintptr_t *buffer, size_t &length)
        {
        }

        /** \brief Unserialises all relevant class data from a buffer.
         *
         * unserialise is called when a buffer needs to be converted into
         * class data. These buffers are filled by serialise calls.
         *
         * Userspace applications generally do not need to implement
         * unserialise, as most unserialisation is done on the kernel's side
         * of the native API.
         *
         * @param buffer address of the buffer to unserialise
         * @param length the length of the buffer
         */
        virtual void unserialise(uintptr_t buffer, size_t &length)
        {
        }
};

#endif
