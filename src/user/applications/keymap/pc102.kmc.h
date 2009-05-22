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

#ifndef PC102_KMC_H
#define PC102_KMC_H

#define UNICODE_POINT    0x80000000
#define STRTAB_ENTRY     0x40000000
#define SPARSE_NULL      0x00000000
#define SPARSE_DATA_FLAG 0x8000

#define NONE_I           0x0
#define SHIFT_I          0x1
#define CTRL_I           0x2

#define ALT_I            0x1
#define ALTGR_I          0x2

#define TABLE_IDX(alt, modifiers, escape, scancode) ( ((alt&3)<<10) | ((modifiers&3)<<8) | ((escape&1)<<7) | (scancode&0x7F) )
#define TABLE_MAX TABLE_IDX(2,3,1,128)

typedef struct table_entry
{
    uint32_t flags;
    uint32_t val;
} table_entry_t;

typedef struct sparse_entry
{
    uint16_t left;
    uint16_t right;
} sparse_t;

char sparse_buff[33] =
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

char data_buff[1] =
"";

char strtab_buff[1] =
"";

#endif
