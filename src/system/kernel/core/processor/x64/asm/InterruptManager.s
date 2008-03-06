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

; X64InterruptManager::interrupt(InterruptState &interruptState)
extern _ZN19X64InterruptManager9interruptER17X64InterruptState

; Export the array of interrupt handler addresses
global interrupt_handler_array

;##############################################################################
;### Code section #############################################################
;##############################################################################
[bits 64]
[section .text]

;##############################################################################
;### assembler stub for interrupt handler #####################################
;##############################################################################
interrupt_handler:
  ; Save the registers
  push rax
  push rbx
  push rcx
  push rdx
  push rdi
  push rsi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  ; Create a new stackframe
  xor rbp, rbp

  ; "Push" the pointer to the InterruptState object
  mov rdi, rsp

  ; Call the C++ Code
  call _ZN19X64InterruptManager9interruptER17X64InterruptState

  ; Restore the registers
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop r8
  pop rbp
  pop rsi
  pop rdi
  pop rdx
  pop rcx
  pop rbx
  pop rax

  ; Remove the errorcode and the interrupt number from the stack
  add rsp, 0x10
  iretq

;##############################################################################
;### Macro for exception handler without error code ###########################
;##############################################################################
%macro INTERRUPT_HANDLER 1
  interrupt%1:
    ; "Push" a fake error code (and interrupt number)
    sub rsp, 0x10

    ; Set the interrupt number
    mov qword [rsp], %1

    ;jmp interrupt_handler
%endmacro

;##############################################################################
;### Macro for exception handler with error code ##############################
;##############################################################################
%macro INTERRUPT_HANDLER_ERRORCODE 1
  interrupt%1:
    ; "Push" and interrupt number
    sub rsp, 0x08

    ; Set the interrupt number
    mov qword [rsp], %1

    jmp interrupt_handler
%endmacro

;##############################################################################
;### All interrupt handler ####################################################
;##############################################################################
INTERRUPT_HANDLER 0
INTERRUPT_HANDLER 1
INTERRUPT_HANDLER 2
INTERRUPT_HANDLER 3
INTERRUPT_HANDLER 4
INTERRUPT_HANDLER 5
INTERRUPT_HANDLER 6
INTERRUPT_HANDLER 7
INTERRUPT_HANDLER_ERRORCODE 8
INTERRUPT_HANDLER 9
INTERRUPT_HANDLER_ERRORCODE 10
INTERRUPT_HANDLER_ERRORCODE 11
INTERRUPT_HANDLER_ERRORCODE 12
INTERRUPT_HANDLER_ERRORCODE 13
INTERRUPT_HANDLER_ERRORCODE 14
INTERRUPT_HANDLER 15
INTERRUPT_HANDLER 16
INTERRUPT_HANDLER_ERRORCODE 17
INTERRUPT_HANDLER 18
INTERRUPT_HANDLER 19
INTERRUPT_HANDLER 20
INTERRUPT_HANDLER 21
INTERRUPT_HANDLER 22
INTERRUPT_HANDLER 23
INTERRUPT_HANDLER 24
INTERRUPT_HANDLER 25
INTERRUPT_HANDLER 26
INTERRUPT_HANDLER 27
INTERRUPT_HANDLER 28
INTERRUPT_HANDLER 29
INTERRUPT_HANDLER 30
INTERRUPT_HANDLER 31
%assign i 32
%rep 224
  INTERRUPT_HANDLER i
  %assign i i+1
%endrep

;##############################################################################
;### Data section #############################################################
;##############################################################################
[section .data]
;##############################################################################
;### Array with the addresses of all interrupt handlers #######################
;##############################################################################
interrupt_handler_array:
  %assign i 0
  %rep 256
    dq interrupt %+ i
    %assign i i+1
  %endrep
