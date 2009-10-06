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

#ifndef ISA_DMA_H
#define ISA_DMA_H

#include <processor/types.h>

/** Base class for ISA DMA implementations.
 *  Different architectures will do things differently, so the base class
 *  is defined here to give a clean interface to the architecture-specific
 *  content.
 *
 *  It is assumed drivers will be notified by some other means that their
 *  data is ready. This class is merely designed to prepare operations on
 *  the DMA controller without requiring drivers to do it themselves.
 */
class IsaDma
{
    public:
        IsaDma()
        {};
        virtual ~IsaDma()
        {};

        static IsaDma &instance();

        /// Initialises a read operation
        virtual bool initTransfer(uint8_t channel, uint8_t mode, size_t length, uintptr_t addr) = 0;

    private:
};

#endif
