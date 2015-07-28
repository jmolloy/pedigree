/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef __X86EMU_FPU_H
#define __X86EMU_FPU_H

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

/* these have to be defined, whether 8087 support compiled in or not. */

extern void x86emuOp_esc_coprocess_d8 (u8 op1);
extern void x86emuOp_esc_coprocess_d9 (u8 op1);
extern void x86emuOp_esc_coprocess_da (u8 op1);
extern void x86emuOp_esc_coprocess_db (u8 op1);
extern void x86emuOp_esc_coprocess_dc (u8 op1);
extern void x86emuOp_esc_coprocess_dd (u8 op1);
extern void x86emuOp_esc_coprocess_de (u8 op1);
extern void x86emuOp_esc_coprocess_df (u8 op1);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_FPU_H */
