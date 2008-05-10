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

; uintptr_t Processor::getBasePointer()
global _ZN9Processor14getBasePointerEv
; uintptr_t Processor::getStackPointer()
global _ZN9Processor15getStackPointerEv
; uintptr_t Processor::getInstructionPointer()
global _ZN9Processor21getInstructionPointerEv
; uintptr_t Processor::getDebugStatus()
global _ZN9Processor14getDebugStatusEv
; void Processor::switchToUserMode()
global _ZN9Processor16switchToUserModeEmm

;##############################################################################
;### Code section #############################################################
;##############################################################################
[bits 32]
[section .text]

_ZN9Processor14getBasePointerEv:
  mov eax, ebp
  ret

_ZN9Processor15getStackPointerEv:
  mov eax, esp
  ret

_ZN9Processor21getInstructionPointerEv:
  mov eax, [esp]
  ret

_ZN9Processor14getDebugStatusEv:
  mov eax, dr6
  ret

_ZN9Processor16switchToUserModeEmm:
  mov ax, 0x23       ; Load the new data segment descriptor with an RPL of 3.
  mov ds, ax         ; Propagate the change to all segment registers.
  mov es, ax
  mov fs, ax
  mov gs, ax

  pop edx            ; Pop the return address - useless now.
  pop edx            ; Pop the EIP to jump to

                     ; The stack now contains the parameter to pass.
  push 0x0           ; Push a dummy return value so the jumped-to procedure can find its parameter.

  mov eax, esp       ; Save the stack pointer before we pushed anything.
  push 0x23          ; push the new stack segment with an RPL of 3.
  push eax           ; Push the value of ESP before we pushed anything.
  pushfd             ; Push the EFLAGS register.

  pop eax            ; Pull the EFLAGS value back.
  or eax, 0x200      ; OR-in 0x200 = 1<<9 = IF = Interrupt flag enable.
  push eax           ; Push the doctored EFLAGS value back.

  push 0x1B          ; Push the new code segment with an RPL of 3.
  push edx           ; Push the EIP to IRET to.

  iret
