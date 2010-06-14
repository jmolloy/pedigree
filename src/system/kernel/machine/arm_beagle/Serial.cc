/*
 * Copyright (c) 2010 Matthew Iselin
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

#include "Serial.h"
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Machine.h>

ArmBeagleSerial::ArmBeagleSerial() : m_Base(0), m_BaseRegion("beagle-uart")
{
}

ArmBeagleSerial::~ArmBeagleSerial()
{
}

void ArmBeagleSerial::setBase(uintptr_t nBaseAddr)
{
    // Map in the base
    if(!PhysicalMemoryManager::instance().allocateRegion(m_BaseRegion,
                                                         1,
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode,
                                                         nBaseAddr))
    {
        // Failed to allocate the region!
        return;
    }

    // Set base MMIO address for UART and configure appropriately.
    m_Base = reinterpret_cast<volatile uint8_t*>(m_BaseRegion.virtualAddress());

    // Reset the UART
    softReset();

    // Restore FIFO defaults
    if(!setFifoDefaults())
    {
        m_Base = 0;
        return;
    }

    // Configure the UART protocol
    if(!configureProtocol())
    {
        m_Base = 0;
        return;
    }

    // Disable hardware flow control
    if(!disableFlowControl())
    {
        m_Base = 0;
        return;
    }

    // Install the IRQ handler
    /// \todo this is the UART3 IRQ... needs to be set for each UART
    InterruptManager::instance().registerInterruptHandler(74, this);
}

void ArmBeagleSerial::interrupt(size_t nInterruptNumber, InterruptState &state)
{
    uint8_t intStatus = m_Base[IIR_REG];
    switch((intStatus >> 1) & 0x1F)
    {
        case 0:
            {
            // Modem interrupt - reset by reading the modem status register
            uint8_t c = m_Base[MSR_REG];
            }
            break;
        case 1:
            {
            // THR interrupt - RX FIFO below threshold or THR empty
            }
            break;
        case 2:
            {
            // RHR interrupt - data ready in the RX FIFO
            while(m_Base[LSR_REG] & 0x1)
                uint8_t c = m_Base[RHR_REG]; /// \todo Queue somewhere for input
            }
            break;
        case 3:
            {
            // Receiver line status error
            /// \todo Handle
            }
            break;
        case 6:
            {
            // RX timeout - caused by stale data in the RX FIFO.
            // Reset by reading from the FIFO
            uint8_t c = m_Base[RHR_REG];
            }
            break;
        case 8:
            {
            // XOFF/special character
            /// \todo Handle
            }
            break;
        case 16:
            {
            // CTS, RTS change of state from active to inactive
            // Reset by the act of reading the IIR, as above
            }
            break;
    }
}

char ArmBeagleSerial::read()
{
    // Read a character from the UART, if we have one configured.
    if(m_Base)
    {
        while(!(m_Base[LSR_REG] & 0x1));
        return m_Base[RHR_REG];
    }
    else
        return 0;
}

char ArmBeagleSerial::readNonBlock()
{
    // Same as above but don't block
    if(m_Base)
    {
        if(!(m_Base[LSR_REG] & 0x1))
            return 0;
        return m_Base[RHR_REG];
    }
    else
        return 0;
}

void ArmBeagleSerial::write(char c)
{
    // Write a character to the UART.
    if(m_Base)
    {
        while(!(m_Base[LSR_REG] & 0x20));
        m_Base[THR_REG] = c;
    }
}

/// \note Page/section references are from the OMAP35xx Technical Reference Manual

void ArmBeagleSerial::softReset()
{
    if(!m_Base)
        return;
        
    /** Reset the UART. Page 2677, section 17.5.1.1.1 **/

    // 1. Initiate a software reset
    m_Base[SYSC_REG] |= 0x2;

    // 2. Wait for the end of the reset operation
    while(!(m_Base[SYSS_REG] & 0x1));
}

bool ArmBeagleSerial::setFifoDefaults()
{
    if(!m_Base)
        return false;
    
    /** Configure FIFOs and DMA **/

    // 1. Switch to configuration mode B to access the EFR_REG register
    unsigned char old_lcr_reg = m_Base[LCR_REG];
    m_Base[LCR_REG] = 0xBF;

    // 2. Enable submode TCR_TLR to access TLR_REG (part 1 of 2)
    unsigned char efr_reg = m_Base[EFR_REG];
    unsigned char old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    m_Base[EFR_REG] = efr_reg; // Write back to the register

    // 3. Switch to configuration mode A
    m_Base[LCR_REG] = 0x80;

    // 4. Enable submode TCL_TLR to access TLR_REG (part 2 of 2)
    unsigned char mcr_reg = m_Base[MCR_REG];
    unsigned char old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    m_Base[MCR_REG] = mcr_reg;

    // 5. Enable FIFO, load new FIFO triggers (part 1 of 3), and the new DMA mode (part 1 of 2)
    m_Base[FCR_REG] = 1; // TX and RX FIFO triggers at 8 characters, no DMA mode

    // 6. Switch to configuration mode B to access EFR_REG
    m_Base[LCR_REG] = 0xBF;

    // 7. Load new FIFO triggers (part 2 of 3)
    m_Base[TLR_REG] = 0;

    // 8. Load new FIFO triggers (part 3 of 3) and the new DMA mode (part 2 of 2)
    m_Base[SCR_REG] = 0;

    // 9. Restore the ENHANCED_EN value saved in step 2
    if(!old_enhanced_en)
        m_Base[EFR_REG] = m_Base[EFR_REG] ^ 0x8;

    // 10. Switch to configuration mode A to access the MCR_REG register
    m_Base[LCR_REG] = 0x80;

    // 11. Restore the MCR_REG TCR_TLR value saved in step 4
    if(!old_tcl_tlr)
        m_Base[MCR_REG] = m_Base[MCR_REG] ^ 0x20;

    // 12. Restore the LCR_REG value stored in step 1
    m_Base[LCR_REG] = old_lcr_reg;

    return true;
}

bool ArmBeagleSerial::configureProtocol()
{
    if(!m_Base)
        return false;
    
    /** Configure protocol, baud and interrupts **/

    // 1. Disable UART to access DLL_REG and DLH_REG
    m_Base[MDR1_REG] = (m_Base[MDR1_REG] & ~0x7) | 0x7;

    // 2. Switch to configuration mode B to access the EFR_REG register
    m_Base[LCR_REG] = 0xBF;

    // 3. Enable access to IER_REG
    unsigned char efr_reg = m_Base[EFR_REG];
    unsigned char old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    m_Base[EFR_REG] = efr_reg; // Write back to the register

    // 4. Switch to operational mode to access the IER_REG register
    m_Base[LCR_REG] = 0;

    // 5. Clear IER_REG
    m_Base[IER_REG] = 0;

    // 6. Switch to configuration mode B to access DLL_REG and DLH_REG
    m_Base[LCR_REG] = 0xBF;

    // 7. Load the new divisor value (looking for 115200 baud)
    m_Base[0x0] = 0x1A; // divisor low byte
    m_Base[0x4] = 0; // divisor high byte

    // 8. Switch to operational mode to access the IER_REG register
    m_Base[LCR_REG] = 0;

    // 9. Load new interrupt configuration
    m_Base[IER_REG] = 0x0D; // RHR interrupt. // No interrupts wanted at this stage

    // 10. Switch to configuration mode B to access the EFR_REG register
    m_Base[LCR_REG] = 0xBF;

    // 11. Restore the ENHANCED_EN value saved in step 3
    if(old_enhanced_en)
        m_Base[EFR_REG] = m_Base[EFR_REG] ^ 0x8;

    // 12. Load the new protocol formatting (parity, stop bit, character length)
    // and enter operational mode
    m_Base[LCR_REG] = 0x3; // 8 bit characters, no parity, one stop bit

    // 13. Load the new UART mode
    m_Base[MDR1_REG] = 0;

    return true;
}

bool ArmBeagleSerial::disableFlowControl()
{
    if(!m_Base)
        return false;
    
    /** Configure hardware flow control */

    // 1. Switch to configuration mode A to access the MCR_REG register
    unsigned char old_lcr_reg = m_Base[LCR_REG];
    m_Base[LCR_REG] = 0x80;

    // 2. Enable submode TCR_TLR to access TCR_REG (part 1 of 2)
    unsigned char mcr_reg = m_Base[MCR_REG];
    unsigned char old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    m_Base[MCR_REG] = mcr_reg;

    // 3. Switch to configuration mode B to access the EFR_REG register
    m_Base[LCR_REG] = 0xBF;

    // 4. Enable submode TCR_TLR to access the TCR_REG register (part 2 of 2)
    unsigned char efr_reg = m_Base[EFR_REG];
    unsigned char old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    m_Base[EFR_REG] = efr_reg; // Write back to the register

    // 5. Load new start and halt trigger values
    m_Base[TCR_REG] = 0;

    // 6. Disable RX/TX hardware flow control mode, and restore the ENHANCED_EN
    // values stored in step 4
    m_Base[EFR_REG] = 0;

    // 7. Switch to configuration mode A to access MCR_REG
    m_Base[LCR_REG] = 0x80;

    // 8. Restore the MCR_REG TCR_TLR value stored in step 2
    if(!old_tcl_tlr)
        m_Base[MCR_REG] = m_Base[MCR_REG] ^ 0x20;

    // 9. Restore the LCR_REG value saved in step 1
    m_Base[LCR_REG] = old_lcr_reg;

    return true;
}

