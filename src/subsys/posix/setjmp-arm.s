# Copyright (c) 2008-2014, Pedigree Developers
# 
# Please see the CONTRIB file in the root of the source tree for a full
# list of contributors.
# 
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

.global setjmp
.global longjmp

.global _setjmp
.global _longjmp

# C function that runs a system call to check to see if we are in a signal
# context, and unwind that context. Allows longjmp from signal handlers.
.extern __pedigree_revoke_signal_context

# jmp_buf:
# v1 - v7, fp, ip, sp, lr, f4, f5, f6, f7

# ldr lr, =_ZN6Thread12threadExitedEv

setjmp:
  bx lr

longjmp:
  # Revoke any existing signal context (in case we are doing a longjmp
  # out of a signal handler.
  # TODO: caller-save registers, preserve parameters, etc
  bl __pedigree_revoke_signal_context

  bx lr

_setjmp:
    b setjmp

_longjmp:
    b longjmp

