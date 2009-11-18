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

#ifndef BUSMASTERIDE_H
#define BUSMASTERIDE_H

/** The Physical Region Descriptor structure */
typedef struct
{
    // First bit = zero
    uint32_t physAddr;

    // First bit = zero
    uint16_t byteCount;

    // Last bit = EOT
    uint16_t rsvdEot;
} __attribute__((packed)) PhysicalRegionDescriptor;

/** Bus Master IDE Registers */
enum BusMasterIdeRegs
{
    PrimaryCommand          = 0,    // R/W
    DeviceSpecific1         = 1,    // --
    PrimaryStatus           = 2,    // R/W/C
    DeviceSpecific2         = 3,    // --
    PrimaryPrdTableAddr     = 4,    // R/W
    SecondaryCommand        = 8,    // R/W
    DeviceSpecific3         = 9,    // --
    SecondaryStatus         = 10,   // R/W
    DeviceSpecific4         = 11,   // --
    SecondaryPrdTableAddr   = 12,   // R/W
};

/** Command register format */
typedef union
{
    uint8_t reg;

    /// \todo I blanked when writing this, so I can't remember if this defines
    ///       "bits" as a packed structure or if the attribute is ignored...
    struct
    {
        uint8_t startStop   : 1;
        uint8_t rsvd1       : 2;
        uint8_t rwCtl       : 1;
        uint8_t rsvd2       : 4;
    } __attribute__((packed)) bits;
} BusMasterIdeCommandRegister;

/** Status register format */
typedef union
{
    uint8_t reg;
    struct bits
    {
        uint8_t bmIdeActive : 1;
        uint8_t error       : 1;
        uint8_t interrupt   : 1;
        uint8_t rsvd : 2;
        uint8_t drv0Capable : 1;
        uint8_t drv1Capable : 1;
        uint8_t simplexOnly : 1;
    } __attribute__((packed)) bits;
} BusMasterIdeStatusRegister;

/** Descriptor table pointer register */
typedef struct
{
    /// Base address of the tab, first two bits must be zero
    uint32_t baseAddress;
} BusMasterIdeDescriptorTablePointerRegister;

#endif
