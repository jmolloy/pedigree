;
; Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
;
; Permission to use, copy, modify, and distribute this software for any
; purpose with or without fee is hereby granted, provided that the above
; copyright notice and this permission notice appear in all copies.
;
; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;

[GLOBAL sigret_stub]
[GLOBAL sigret_stub_end]

; void sigret_stub(size_t p1, size_t p2, size_t p3, size_t p4);
sigret_stub:

  ; Skip the return address
  add esp, 4

  ; Grab the serialised data, read the first byte for the signal number
  mov edi, [esp]
  mov esi, [esp + 4]
  cmp esi, 0
  jz .justRun

  xor ebx, ebx
  mov byte bl, [esi]
  push ebx

.justRun:

  ; Run the handler
  call edi

  ; Return to the kernel
  mov eax, 0x10066
  int 0xff

sigret_stub_end:
