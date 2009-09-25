;
; Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

[global updateBuffer_15bpp_5r5g5b]
[global updateBuffer_16bpp_5r6g5b]
[global updateBuffer_24bpp_8r8g8b]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Update buffer, 15 BPP
        
;;; [esp+28] target buffer
;;; [esp+24] width
;;; [esp+20] y2
;;; [esp+16] x2
;;; [esp+12] y1
;;; [esp+8]  x1
;;; [esp+4]  pBuffer
updateBuffer_15bpp_5r5g5b:
        push ebp
        push ebx
        push esi
        push edi

        ;; pBuffer into ebx.
        mov ebx, [esp+20]
        ;; Y1 into ecx.
        mov ecx, [esp+28]
        ;; Y2 into edx.
        mov edx, [esp+36]
        inc edx                 ; Makes testing <= easier, +1, then just test <.
        ;; X1 into esi.
        mov esi, [esp+16]
        ;; X2 into edi.
        mov edi, [esp+32]
        inc edi                 ; See above.

        ;; Outer loop: y1..y2
.start_outer:
        cmp ecx, edx
        je .finish_outer

        ;; Set %esi as X1 again.
        mov esi, [esp+16]
        
        ;; Ensure we can use EDX in the inner function.
        push edx

        ;; Generate array subscript: (Y*width) + X = %ecx + %esi
        mov eax, ecx
        add eax, esi
        ;; Save array subscript in %ebx for target pointer.
        mov ebx, eax
        ;; Each item in source array is 3 bytes long. NOTE: trashes %edx.
        mul 3
        add eax, [esp+24]

        ;; Each item in target array is 2 bytes long.
        shl ebx, 2
        add ebx, [esp+48]

        ;; Now %eax is the source ptr, %ebx the target.
        
        ;; Inner loop:  x1..x2
.start_inner:
        cmp esi, edi
        je .finish_inner

        ;; Main function. Scratch register: edx.

        ;; Zero edx, this will become our accumulator.
        xor edx, edx

        ;; R field, 5 bits long.
        mov dl, [eax+0]
        shr dl, 3               ; 8 bits down to 5.
        shl dx, 8               ; We're short on registers, so save R field up in dh.

        ;; G field. 5 bits long.
        mov dl, [eax+1]
        and dl, 0xF8
        shl dx, 2              ; Shunt the R and G fields to where they're supposed to be.

        ;; B field. 5 bits long
        mov bp, [eax+2]
        shr bp, 3
        and bp, 0x001F          ; Remove all but the bottom 5 bits.
        or  bp, dx              ; Transplant the R and G fields onto the B field.
        
        mov [ebx], bp

        inc esi
        add eax, 3
        add ebx, 2
        jmp .start_inner

.finish_inner:
        pop ecx
        add ecx, [esp+40]
        jmp .start_outer

.finish_outer:
        pop edi
        pop esi
        pop ebx
        pop ebp

        ret
