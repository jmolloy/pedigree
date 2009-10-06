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

#ifndef X86_ISA_DMA_H
#define X86_ISA_DMA_H

#include <processor/types.h>
#include <processor/IoPort.h>
#include "../IsaDma.h"

/** ISA DMA (PC) Master chip */
namespace MasterChip
{
    enum MasterRegs
    {
        Status = 0xD0,          // Read
        Command = 0xD0,         // Write
        Request = 0xD2,         // Write
        ChannelMask = 0xD4,     // Write
        Mode = 0xD6,            // Write
        ByteWord = 0xD8,        // Read
        Intermediate = 0xDA,    // Read
        Mask = 0xDE,            // Write
    };

    enum BaseRegisters {
        AddressChannel0_4 = 0xC0,
        CountChannel0_4 = 0xC1,

        AddressChannel1_5 = 0xC2,
        CountChannel1_5 = 0xC3,

        AddressChannel2_6 = 0xC4,
        CountChannel2_6 = 0xC5,

        AddressChannel3_7 = 0xC6,
        CountChannel3_7 = 0xC7,
    };
}

/** ISA DMA (PC) Slave chip */
namespace SlaveChip
{
    enum SlaveRegs
    {
        Status = 0x08,          // Read
        Command = 0x08,         // Write
        Request = 0x09,         // Write
        ChannelMask = 0x0A,     // Write
        Mode = 0x0B,            // Write
        ByteWord = 0x0C,        // Read
        Intermediate = 0x0D,    // Read
        Mask = 0x0F,            // Write
    };

    enum BaseRegisters {
        AddressChannel0_4 = 0x00,
        CountChannel0_4 = 0x01,

        AddressChannel1_5 = 0x02,
        CountChannel1_5 = 0x03,

        AddressChannel2_6 = 0x04,
        CountChannel2_6 = 0x05,

        AddressChannel3_7 = 0x05,
        CountChannel3_7 = 0x07,
    };
}

/** ISA DMA (PC) Page address registers */
namespace PageRegisters
{
    enum PageAddressRegister {
        Channel0 = 0x87,
        Channel1 = 0x83,
        Channel2 = 0x81,
        Channel3 = 0x82,
        Channel4 = 0x8F,
        Channel5 = 0x8B,
        Channel6 = 0x89,
        Channel7 = 0x8A,
    };
}

/** x86 ISA DMA implementations.
 *  It is assumed drivers will be notified by some other means that their
 *  data is ready. This class is merely designed to prepare operations on
 *  the DMA controller without requiring drivers to do it themselves.
 */
class X86IsaDma : public IsaDma
{
    public:

        enum TransferModes
        {
            SelfTest = 0,
            Write = (1 << 2),
            Read = (2 << 2),
            Cascade = (3 << 2),
            AutoInit = (1 << 4),
            Increment = 0,
            Decrement = (1 << 5),
            OnDemand = 0,
            Single = (1 << 6),
            Block = (2 << 6)
        };

        X86IsaDma() : m_Io("isa-dma")
        {
            if(!m_Io.allocate(0, 0x100))
                ERROR("X86IsaDma: Couldn't allocate port range");
        };
        virtual ~X86IsaDma()
        {};

        static X86IsaDma &instance()
        {
            return m_Instance;
        }

        /// Initialises an operation
        virtual bool initTransfer(uint8_t channel, uint8_t mode, size_t length, uintptr_t addr);

    private:

        void resetFlipFlop(uint8_t chan);

        void resetHard(uint8_t chan);

        void unmaskAll();

        bool internalSetup(uint8_t channel, size_t length, uintptr_t addr);

        IoPort m_Io;

        static X86IsaDma m_Instance;
};

#endif
