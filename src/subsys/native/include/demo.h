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

#ifndef _PEDIGREE_DEMO_H
#define _PEDIGREE_DEMO_H

#include <native/types.h>

#include <native/Object.h>

/** Object is merely the base class for all native subsystem objects. */
class Demo : public Object
{
    public:
        Demo();
        virtual ~Demo();

        void something();

        /**
         * Retrieves the global unique identifier for this class.
         *
         * The global unique identifier is used when creating a new object to
         * register the application object with the kernel. This identifier
         * must match with the kernel's mapping of identifiers to kernel classes
         * otherwise the system calls will fail in unexpected ways.
         */
        virtual uint64_t guid() const
        {
            return 0xdeadbeef;
        }
};

#endif
