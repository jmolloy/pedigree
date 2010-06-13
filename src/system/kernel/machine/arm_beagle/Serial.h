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

#ifndef MACHINE_ARM_BEAGLE_SERIAL_H
#define MACHINE_ARM_BEAGLE_SERIAL_H

#include <machine/Serial.h>
#include <processor/MemoryRegion.h>
#include <processor/InterruptManager.h>

/// Implements the UART interface on the BeagleBoard
class ArmBeagleSerial : public Serial, public InterruptHandler
{
    public:
      ArmBeagleSerial();
      virtual ~ArmBeagleSerial();
  
      virtual void setBase(uintptr_t nBaseAddr);
      virtual char read();
      virtual char readNonBlock();
      virtual void write(char c);

      virtual void interrupt(size_t nInterruptNumber, InterruptState &state);

    private:

        /// Perform a software reset of this UART
        void softReset();

        /// Reset the FIFOs and DMA to default values
        bool setFifoDefaults();

        /// Configure the UART protocol to defaults - 115200 baud, 8 character
        /// bits, no parity, 1 stop bit. Enables the UART for output as a side
        /// effect.
        bool configureProtocol();

        /// Disables hardware flow control on the UART
        bool disableFlowControl();

        enum RegisterOffsets
        {
            DLL_REG        = 0x00, // R/W
            RHR_REG        = 0x00, // R
            THR_REG        = 0x00, // W
            DLH_REG        = 0x04, // R/W
            IER_REG        = 0x04, // R/W
            IIR_REG        = 0x08, // R
            FCR_REG        = 0x08, // W
            EFR_REG        = 0x08, // RW
            LCR_REG        = 0x0C, // RW
            MCR_REG        = 0x10, // RW
            XON1_ADDR1_REG = 0x10, // RW
            LSR_REG        = 0x14, // R
            XON2_ADDR2_REG = 0x14, // RW
            MSR_REG        = 0x18, // R
            TCR_REG        = 0x18, // RW
            XOFF1_REG      = 0x18, // RW
            SPR_REG        = 0x1C, // RW
            TLR_REG        = 0x1C, // RW
            XOFF2_REG      = 0x1C, // RW
            MDR1_REG       = 0x20, // RW
            MDR2_REG       = 0x24, // RW

            USAR_REG       = 0x38, // R

            SCR_REG        = 0x40, // RW
            SSR_REG        = 0x44, // R

            MVR_REG        = 0x50, // R
            SYSC_REG       = 0x54, // RW
            SYSS_REG       = 0x58, // R
            WER_REG        = 0x5C, // RW
        };

        /** Base address for MMIO */
        volatile uint8_t *m_Base;

        /** MemoryRegion for the MMIO base */
        MemoryRegion m_BaseRegion;
};

#endif
