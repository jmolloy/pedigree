/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef MACHINE_AU1500_SERIAL_H
#define MACHINE_AU1500_SERIAL_H

#include <machine/Serial.h>

/**
 * Implements a driver for the Serial connection of the malta's UART.
 * 
 * Base for UART registers: 0x11100000
 * Register offsets:
 * RXDATA:   0x00  Receive char fifo.
 * TXDATA:   0x04  Transmit char fifo.
 * INTEN:    0x08  Interrupt enable.
 * INTCAUSE: 0x0C  Pending interrupt cause register.
 * FIFOCTRL: 0x10  FIFO control register.
 * LINECTRL: 0x14  Line control register.
 * MDMCTRL:  0x18  Modem line control register (UART3 only).
 * LINESTAT: 0x1C  Line status register.
 * MDMSTAT:  0x20  Modem status register (UART3 only).
 * AUTOFLOW: 0x24  Automatic Hardware flow control (UART3 only).
 * CLKDIV:   0x28  Baud rate clock divider.
 * ENABLE:   0x100 Module Enable register.
 */
class Au1500Serial : public Serial
{
  public:
    Au1500Serial();
    virtual ~Au1500Serial();
  
    virtual void setBase(uintptr_t nBaseAddr);
    virtual char read();
    virtual char readNonBlock();
    virtual void write(char c);
    
    struct serial
    {
      uint32_t rxdata;
      uint32_t txdata;
      uint32_t inten;
      uint32_t intcause;
      uint32_t fifoctrl;
      uint32_t linectrl;
      uint32_t mdmstat;
      uint32_t autoflow;
      uint32_t clkdiv;
      uint32_t enable;
    } PACKED;
    
    /**
     * The serial device's registers.
     */
    volatile serial *m_pRegs;
};

#endif
