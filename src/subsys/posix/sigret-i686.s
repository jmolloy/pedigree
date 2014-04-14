; Copyright (c) 2008-2014, Pedigree Developers
; 
; Please see the CONTRIB file in the root of the source tree for a full
; list of contributors.
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

[GLOBAL sigret_stub]
[GLOBAL sigret_stub_end]

; void sigret_stub(size_t p1, size_t p2, size_t p3, size_t p4);
sigret_stub:

  ; Skip the return address
  add esp, 4

  ; Grab the serialised data, read the first byte for the signal number
  ;mov edi, [esp]
  ;mov esi, [esp + 4]
  ;cmp esi, 0
  ;jz .justRun

  ;xor ebx, ebx
  ;mov byte bl, [esi]
  ;push ebx
  
  ; Handler address
  mov edi, [esp]
  
  ; Push the serialized buffer
  xor eax, eax
  
  mov esi, [esp + 4]
  cmp esi, 0
  jz .justRun
  mov eax, [esi]
  and eax, 0xff
  
  ; TODO: Push the handler address too and call a common stub of some sort...

.justRun:
  push eax
  
  ; Run the handler
  call edi

  ; Return to the kernel
  mov eax, 0x10065
  int 0xff

  jmp $

sigret_stub_end:

[GLOBAL pthread_stub]
[GLOBAL pthread_stub_end]
[EXTERN pthread_exit]

; void pthread_stub(size_t p1, size_t p2, size_t p3, size_t p4);
pthread_stub:

  ; Skip the return address
  add esp, 4

  ; Call the entry function
  mov ebx, 0
  mov eax, 0x10051
  int 0xff

  ; First parameter: entry point
  mov esi, [esp]

  ; Second parameter: argument
  mov edi, [esp + 4]
  push edi

  ; Run the function
  call esi

  ; Effectively, pthread_exit... We can't refer to libc functions from here,
  ; as this code is copied to userspace at runtime.
  mov ebx, eax
  mov eax, 0x10050
  int 0xff

  jmp $

pthread_stub_end:
