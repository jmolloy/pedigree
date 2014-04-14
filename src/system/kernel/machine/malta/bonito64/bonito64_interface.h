/*
 * 
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

#ifndef __BONITO64_INTERFACE_H
#define __BONITO64_INTERFACE_H

class Bonito64Interface
{
public:
    Bonito64Interface() {}
    virtual ~Bonito64Interface() {}

    virtual IrqManager* getIrqManager() = 0;
    virtual size_t getNumSerial() = 0;
    virtual size_t getNumVga() = 0;
    virtual SchedulerTimer* getSchedulerTimer() = 0;
    virtual Serial* getSerial(size_t n) = 0;
    virtual Timer* getTimer() = 0;
    virtual Vga* getVga(size_t n) = 0;
    virtual void initialise() = 0;

private:
    Bonito64Interface( const Bonito64Interface& source );
    void operator = ( const Bonito64Interface& source );
};


#endif // __BONITO64_INTERFACE_H
