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

#include "X86IsaDma.h"

X86IsaDma X86IsaDma::m_Instance;

bool X86IsaDma::initTransfer(uint8_t channel, uint8_t mode, size_t length, uintptr_t addr)
{
    /// \todo Extra flags to control modes and things
    // NOTICE("X86IsaDma::initRead(" << channel << ", " << mode << ", " << length << ", " << addr << ")");

    // The DMA chip will transfer one extra byte... So compensate.
    length--;

    // Setup the internal state
    if(!internalSetup(channel, length, addr))
    {
        NOTICE("Internal setup failed");
        return false;
    }

    // Mask our DMA channel
    size_t port = 0, port2 = 0;
    if(channel < 4)
    {
        port = SlaveChip::ChannelMask;
        port2 = SlaveChip::Mode;
    }
    else
    {
        port = MasterChip::ChannelMask;
        port2 = MasterChip::Mode;
    }
    m_Io.write8(0x4 | (channel & 0x3), port);

    // Send the... command?
    uint8_t modeReg = 0;
    modeReg |= channel & 0x3;
    modeReg |= (mode & 0x0C); // Transfer type
    modeReg |= (mode & 0xC0);
    m_Io.write8(modeReg, port2);

    // Unmask the channel
    m_Io.write8(channel & 0x3, port);

    // Successfully set up the transfer
    return true;
}

void X86IsaDma::resetFlipFlop(uint8_t chan)
{
    if(chan < 4)
        m_Io.write8(0xff, SlaveChip::ByteWord);
    else
        m_Io.write8(0xff, MasterChip::ByteWord);
}

void X86IsaDma::resetHard(uint8_t chan)
{
    if(chan < 4)
        m_Io.write8(0xff, SlaveChip::Intermediate);
    else
        m_Io.write8(0xff, MasterChip::Intermediate);
}

void X86IsaDma::unmaskAll()
{
    m_Io.write8(0xff, 0xdc);
}

bool X86IsaDma::internalSetup(uint8_t channel, size_t length, uintptr_t addr)
{
    // Some channels are not supposed to be used...
    if(channel == 0 || channel == 4)
        return false;

    // May only read up to 64kb of data
    if(length > 0x10000)
        return false;

    // Master and slave chips have different registers, as do different channels.
    // This helps keep things clean.
    size_t port = 0, port2 = 0;

    // Mask the channel we're going to use
    if(channel < 4)
        port = SlaveChip::ChannelMask;
    else
        port = MasterChip::ChannelMask;
    m_Io.write8(0x4 | (channel & 0x3), port);

    // Reset the flip-flop (16-bit transfer coming)
    resetFlipFlop(channel);

    // Grab the address register port
    if(channel == 1)
    {
        port = SlaveChip::AddressChannel1_5;
        port2 = SlaveChip::CountChannel1_5;
    }
    else if(channel == 2)
    {
        port = SlaveChip::AddressChannel2_6;
        port2 = SlaveChip::CountChannel2_6;
    }
    else if(channel == 3)
    {
        port = SlaveChip::AddressChannel3_7;
        port2 = SlaveChip::CountChannel3_7;
    }
    else if(channel == 5)
    {
        port = MasterChip::AddressChannel1_5;
        port2 = MasterChip::CountChannel1_5;
    }
    else if(channel == 6)
    {
        port = MasterChip::AddressChannel2_6;
        port2 = MasterChip::CountChannel2_6;
    }
    else if(channel == 7)
    {
        port = MasterChip::AddressChannel3_7;
        port2 = MasterChip::CountChannel3_7;
    }
    else
        return false;

    // Write first 16 bytes of the address
    m_Io.write8(addr & 0xFF, port);
    m_Io.write8((addr >> 8) & 0xFF, port);

    // Reset the flip-flop (another 16-bit transfer coming)
    resetFlipFlop(channel);

    // Size of the buffer
    m_Io.write8(length & 0xFF, port2);
    m_Io.write8((length >> 8) & 0xFF, port2);

    // Page register port
    if(channel == 1)
        port = PageRegisters::Channel1;
    else if(channel == 2)
        port = PageRegisters::Channel2;
    else if(channel == 3)
        port = PageRegisters::Channel3;
    else if(channel == 5)
        port = PageRegisters::Channel5;
    else if(channel == 6)
        port = PageRegisters::Channel6;
    else if(channel == 7)
        port = PageRegisters::Channel7;
    else
        FATAL("X86 ISA DMA: Somehow channel is zero or four - impossible!");

    // External page register (nth 64k block)
    uint8_t extPageReg = addr / 0x10000;
    m_Io.write8(extPageReg, port);

    // Unmask the channel we've used
    if(channel < 4)
        port = SlaveChip::ChannelMask;
    else
        port = MasterChip::ChannelMask;
    m_Io.write8(channel & 0x3, port);

    // Success!
    return true;
}
