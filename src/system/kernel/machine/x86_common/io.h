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

#ifndef KERNEL_MACHINE_X86_COMMON_IO_H
#define KERNEL_MACHINE_X86_COMMON_IO_H

uint8_t IoPort::in8(size_t offset)
{
  uint8_t value;
  asm volatile("inb %%dx, %%al":"=a" (value):"d" (m_IoPort + offset));
  return value;
}
uint16_t IoPort::in16(size_t offset)
{
  uint16_t value;
  asm volatile("inw %%dx, %%ax":"=a" (value):"d" (m_IoPort + offset));
  return value;
}
uint32_t IoPort::in32(size_t offset)
{
  uint32_t value;
  asm volatile("in %%dx, %%eax":"=a" (value):"d" (m_IoPort + offset));
  return value;
}
void IoPort::out8(uint8_t value, size_t offset)
{
  asm volatile("outb %%al, %%dx"::"d" (m_IoPort + offset), "a" (value));
}
void IoPort::out16(uint16_t value, size_t offset)
{
  asm volatile("outw %%ax, %%dx"::"d" (m_IoPort + offset), "a" (value));
}
void IoPort::out32(uint32_t value, size_t offset)
{
  asm volatile("out %%eax, %%dx"::"d" (m_IoPort + offset), "a" (value));
}

#endif
