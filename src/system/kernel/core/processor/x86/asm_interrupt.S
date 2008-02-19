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

; X86InterruptManager::interrupt(InterruptState &interruptState)
extern _ZN19X86InterruptManager9interruptERN3x8614InterruptStateE

; Export the array of interrupt handler addresses
global interrupt_handler_array

;##############################################################################
;### Code section #############################################################
;##############################################################################
[bits 32]
[section .text]

;##############################################################################
;### assembler stub for interrupt handler #####################################
;##############################################################################
interrupt_handler:
  ; Save the registers
  pusha

  ; Switch to the kernel data segment
  mov ax, 0x10
  mov ds, ax

  ; Create a new stackframe
  xor ebp, ebp

  ; Push the pointer to the InterruptState object
  mov eax, esp
  push eax

  ; Call the C++ Code
  call _ZN19X86InterruptManager9interruptERN3x8614InterruptStateE

  ; Cleanup the stack
  add esp, 0x04

  ; Switch to the usermode data segment
  mov ax, 0x23
  mov ds, ax

  ; Restore the registers
  popa

  ; Remove the errorcode and the interrupt number from the stack
  add esp, 0x08
  iret

;##############################################################################
;### Macro for exception handler without error code ###########################
;##############################################################################
%macro INTERRUPT_HANDLER 1
  interrupt%1:
    ; Push a fake error code
    push dword 0

    ; Push the interrupt number
    push dword %1

    jmp interrupt_handler
%endmacro

;##############################################################################
;### Macro for exception handler with error code ##############################
;##############################################################################
%macro INTERRUPT_HANDLER_ERRORCODE 1
  interrupt%1:
    ; Push the interrupt number
    push dword %1

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
    dd interrupt %+ i
    %assign i i+1
  %endrep
