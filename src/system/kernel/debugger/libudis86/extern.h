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

#ifndef UD_EXTERN_H
#define UD_EXTERN_H

#ifdef __cplusplus
extern "C" {
#endif

//include <stdio.h>
#include "types.h"

/* ============================= PUBLIC API ================================= */

extern void ud_init(struct ud*);

extern void ud_set_mode(struct ud*, uint8_t);

extern void ud_set_pc(struct ud*, uint64_t);

extern void ud_set_input_hook(struct ud*, int (*)(struct ud*));

extern void ud_set_input_buffer(struct ud*, uint8_t*, size_t);

#ifndef __UD_STANDALONE__
extern void ud_set_input_file(struct ud*, FILE*);
#endif /* __UD_STANDALONE__ */

extern void ud_set_vendor(struct ud*, unsigned);

extern void ud_set_syntax(struct ud*, void (*)(struct ud*));

extern void ud_input_skip(struct ud*, size_t);

extern int ud_input_end(struct ud*);

extern unsigned int ud_decode(struct ud*);

extern unsigned int ud_disassemble(struct ud*);

extern void ud_translate_intel(struct ud*);

extern void ud_translate_att(struct ud*);

extern char* ud_insn_asm(struct ud* u);

extern uint8_t* ud_insn_ptr(struct ud* u);

extern uint64_t ud_insn_off(struct ud*);

extern char* ud_insn_hex(struct ud*);

extern unsigned int ud_insn_len(struct ud* u);

extern const char* ud_lookup_mnemonic(enum ud_mnemonic_code c);

/* ========================================================================== */

#ifdef __cplusplus
}
#endif
#endif
