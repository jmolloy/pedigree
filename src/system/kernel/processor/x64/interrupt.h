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
#ifndef KERNEL_PROCESSOR_X64_INTERRUPT_H
#define KERNEL_PROCESSOR_X64_INTERRUPT_H

#include <processor/types.h>

namespace x64
{
  /** @addtogroup kernelprocessorx64 x64
   * x64 processor-specific kernel
   *  @ingroup kernelprocessor
   * @{ */

  /** Structure of a x64 long-mode gate descriptor */
  struct gate_descriptor
  {
    /** Bits 0-15 of the offset */
    uint16_t offset0;
    /** The segment selector */
    uint16_t selector;
    /** Entry number in the interrupt-service-table */
    uint8_t ist;
    /** Flags */
    uint8_t flags;
    /** Bits 16-31 of the offset */
    uint16_t offset1;
    /** Bits 32-63 of the offset */
    uint32_t offset2;
    /** Reserved, must be 0 */
    uint32_t res;
  } __attribute__((packed));

  /** @} */
}

#endif
