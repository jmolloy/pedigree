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

#ifndef MACHINE_X86_VGA_H
#define MACHINE_X86_VGA_H

#define VGA_BASE                0x3C0
#define VGA_AC_INDEX            0x0
#define VGA_AC_WRITE            0x0
#define VGA_AC_READ             0x1
#define VGA_MISC_WRITE          0x2
#define VGA_SEQ_INDEX           0x4
#define VGA_SEQ_DATA            0x5
#define VGA_DAC_READ_INDEX      0x7
#define VGA_DAC_WRITE_INDEX     0x8
#define VGA_DAC_DATA            0x9
#define VGA_MISC_READ           0xC
#define VGA_GC_INDEX            0xE
#define VGA_GC_DATA             0xF
/*                              COLOR emulation  MONO emulation */
#define VGA_CRTC_INDEX          0x14             /* 0x3B4 */
#define VGA_CRTC_DATA           0x15             /* 0x3B5 */
#define VGA_INSTAT_READ         0x1A

#define VGA_NUM_SEQ_REGS        5
#define VGA_NUM_CRTC_REGS       25
#define VGA_NUM_GC_REGS         9
#define VGA_NUM_AC_REGS         21
#define VGA_NUM_REGS            (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
                                VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

void vga_init();

#endif
