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

ArmBeagleSerial::ArmBeagleSerial()
{
}
ArmBeagleSerial::~ArmBeagleSerial()
{
}
void ArmBeagleSerial::setBase(uintptr_t nBaseAddr)
{
    // Set base MMIO address for UART and configure appropriately.
}
char ArmBeagleSerial::read()
{
    // Read a character from the UART, if we have one configured.
}
char ArmBeagleSerial::readNonBlock()
{
    // Same as above but don't block
}
void ArmBeagleSerial::write(char c)
{
    // Write a character to the UART.
}

