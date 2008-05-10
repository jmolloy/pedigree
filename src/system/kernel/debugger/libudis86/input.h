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

#ifndef UD_INPUT_H
#define UD_INPUT_H

#include "types.h"

uint8_t inp_next(struct ud*);
uint8_t inp_peek(struct ud*);
uint8_t inp_uint8(struct ud*);
uint16_t inp_uint16(struct ud*);
uint32_t inp_uint32(struct ud*);
uint64_t inp_uint64(struct ud*);
void inp_move(struct ud*, size_t);
void inp_back(struct ud*);

/* inp_init() - Initializes the input system. */
#define inp_init(u) \
do { \
  u->inp_curr = 0; \
  u->inp_fill = 0; \
  u->inp_ctr  = 0; \
  u->inp_end  = 0; \
} while (0)

/* inp_start() - Should be called before each de-code operation. */
#define inp_start(u) u->inp_ctr = 0

/* inp_back() - Resets the current pointer to its position before the current
 * instruction disassembly was started.
 */
#define inp_reset(u) \
do { \
  u->inp_curr -= u->inp_ctr; \
  u->inp_ctr = 0; \
} while (0)

/* inp_sess() - Returns the pointer to current session. */
#define inp_sess(u) (u->inp_sess)

/* inp_cur() - Returns the current input byte. */
#define inp_curr(u) ((u)->inp_cache[(u)->inp_curr])

#endif
