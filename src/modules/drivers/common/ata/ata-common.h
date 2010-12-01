/*
 * Copyright (c) 2010 James Molloy, Jörg Pfähler, Matthew Iselin
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
#ifndef _ATA_COMMON_H
#define _ATA_COMMON_H

#include <processor/types.h>
#include <processor/IoBase.h>

typedef union
{
    /// This struct provides an easy way to literally code when working with
    /// the ATA status register.
    struct {
        uint32_t err : 1;
        uint32_t obs1 : 1;
        uint32_t obs2 : 1;
        uint32_t drq : 1;
        uint32_t rsvd1 : 1;
        uint32_t rsvd2 : 1;
        uint32_t drdy : 1;
        uint32_t bsy : 1;
    } __attribute__((packed)) reg;

    /// "Hidden" integer which contains the actual register contents
    uint8_t __reg_contents;
} AtaStatus;

/// Performs a proper wait for the drive to become ready as per the ATA spec
inline AtaStatus ataWait(IoBase *pBase)
{
    // Grab the status register first (of course)
    AtaStatus ret;
    uint8_t status = pBase->read8(7);
    if(status == 0)
    {
        ret.__reg_contents = status;
        return ret;
    }

    // Wait for BSY to be unset. Until BSY is unset, no other bits in the
    // register are considered valid.
    while(status & 0x80)
        status = pBase->read8(7);

    // We no longer check DRQ as some commands depend on DRQ being set.
    // For example, a data transfe uses DRQ to say data is available for
    // reading.

    // And now verify that DRDY or ERR are asserted (or both!), but only
    // if DRQ is not set. DRDY will never be set if DRQ is set, and ERR
    // will come out in the return value even if DRQ is set.
    if(!(status & 0x8))
    {
        while(!(status & 0x41))
        {
            status = pBase->read8(7);
            
            // If for some reason the original check for zero gave back something
            // set, we check again here so we don't infinitely loop waiting for
            // the bits that'll never be set. Essentially an escape for devices
            // that aren't present.
            if(!status)
                break;
        }
    }

    // Okay, BSY is unset now. The drive is no longer busy, it is up to the
    // caller to read the status and verify further bits (eg, ERR)
    ret.__reg_contents = status;
    return ret;
}

#endif
