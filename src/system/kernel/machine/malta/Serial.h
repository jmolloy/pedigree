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
#ifndef SERIAL_H
#define SERIAL_H

#include <compiler.h>
#include <processor/types.h>
#include <machine/Serial.h>

/**
 * Implements a driver for the Serial connection of the malta's UART.
 * 
 * Base for UART registers: 0x1F000900
 * Register offsets:
 * RXTX:    0x00  Receive/Transmit char register.
 * INTEN:   0x08  Interrupt enable register.
 * IIFIFO:  0x10  Read: Interrupt identification, Write: FIFO control.
 * LCTRL:   0x18  Line control register.
 * MCTRL:   0x20  Modem control register.
 * LSTAT:   0x28  Line status register.
 * MSTAT:   0x30  Modem status register.
 * SCRATCH: 0x38  Scratch register.
 */
class MaltaSerial : public Serial
{
public:
  /**
   * Default constructor, does nothing.
   */
  MaltaSerial();
  /**
   * Destructor.
   */
  ~MaltaSerial();

  virtual void setBase(uintptr_t nBaseAddr);
  
  /**
   * Writes a character out to the serial port.
   * \note Blocking I/O.
   */
  virtual void write(char c);
  
  /**
   * Reads a character in from the serial port.
   * \note Blocking I/O.
   * \return Zero on a non-printable character without a hardcoded VT100 string,
   *         the next character in the buffer otherwise.
   */
  virtual char read();
  virtual char readNonBlock();
private:
  MaltaSerial(const MaltaSerial &);
  MaltaSerial &operator = (const MaltaSerial &);
  
  struct serial
  {
    uint8_t rxtx;
    uint8_t inten;
    uint8_t iififo;
    uint8_t lctrl;
    uint8_t mctrl;
    uint8_t lstat;
    uint8_t mstat;
    uint8_t scratch;
  } PACKED;
  /**
   * If we're returning a VT100 string (more than one char), this buffer pointer can be
   * set to the current location in the string. If it is NULL, the next character is pulled from
   * the port.
   */
  const char *m_pBuffer;
  
  /**
   * The serial device's registers.
   */
  volatile serial *m_pRegs;
};

#endif
