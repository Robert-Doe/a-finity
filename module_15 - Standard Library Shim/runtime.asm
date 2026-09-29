; runtime.asm — Standard library shim for my_compiler
; Module 15: Standard Library Shim
;
; Provides: print_int, print_newline
;
; Assemble: nasm -f elf64 runtime.asm -o runtime.o
; Link:     gcc -no-pie program.o runtime.o -o program
;
; Uses the Linux write(2) syscall directly (no libc dependency):
;   rax = 1  (sys_write)
;   rdi = 1  (fd = stdout)
;   rsi = buffer pointer
;   rdx = byte count

section .text

global print_int
global print_newline

; -----------------------------------------------------------------------
; print_int(long x)  — rdi = integer to print followed by newline
;
; Algorithm:
;   1. Handle sign: if negative, print '-', negate value.
;   2. Convert absolute value to decimal digits by repeated division by 10.
;      Digits arrive in reverse order; store them right-to-left in a stack
;      buffer so the first digit is at the lowest address when done.
;   3. Append a newline byte after the last digit.
;   4. Call sys_write once for the entire string (digits + newline).
;
; Stack layout (relative to rbp, after "push rbp / mov rbp,rsp / sub rsp,48"):
;   [rbp - 48] .. [rbp - 2]  : 47-byte digit buffer (built right-to-left)
;   [rbp - 1]                 : newline byte (written at entry, stays fixed)
;
; We use rcx as the "write pointer" that walks leftward through the buffer.
; r8  = saved end pointer (rbp-2, last digit slot before the newline)
; r9b = sign flag (1 = negative)
; r10 = divisor (10)
; -----------------------------------------------------------------------
print_int:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 48

    ; Place the newline at the very end of the buffer
    mov     byte [rbp - 1], 10          ; '\n'

    ; rcx = write pointer (starts at second-to-last slot)
    lea     rcx, [rbp - 2]
    mov     r8, rcx                     ; r8 = end-of-digits pointer

    ; Load the argument
    mov     rax, rdi

    ; Handle zero as a special case
    test    rax, rax
    jnz     .nonzero
    mov     byte [rcx], '0'
    dec     rcx
    jmp     .write

.nonzero:
    ; Determine sign
    xor     r9d, r9d                    ; r9b = 0 (positive)
    test    rax, rax
    jns     .positive
    mov     r9b, 1                      ; mark negative
    neg     rax                         ; work with positive magnitude

.positive:
    mov     r10, 10

.digit_loop:
    xor     rdx, rdx
    div     r10                         ; rax = quotient, rdx = remainder
    add     dl, '0'
    mov     [rcx], dl
    dec     rcx
    test    rax, rax
    jnz     .digit_loop

    ; Prepend '-' if negative
    test    r9b, r9b
    jz      .write
    mov     byte [rcx], '-'
    dec     rcx

.write:
    ; rcx currently points ONE BEFORE the first character.
    ; Advance it so it points AT the first character.
    inc     rcx

    ; Compute length: from rcx to (rbp-1) inclusive
    ;   length = (rbp - 1) - rcx + 1  = rbp - rcx
    lea     rdx, [rbp - 1]             ; rdx = address of newline
    sub     rdx, rcx                   ; rdx = length of digits
    inc     rdx                        ; include the newline byte

    mov     rax, 1                     ; sys_write
    mov     rdi, 1                     ; fd = stdout
    mov     rsi, rcx                   ; buffer = start of digit string
    syscall

    mov     rsp, rbp
    pop     rbp
    ret

; -----------------------------------------------------------------------
; print_newline() — print a single newline to stdout
; -----------------------------------------------------------------------
print_newline:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 16

    mov     byte [rbp - 1], 10         ; '\n'
    mov     rax, 1                     ; sys_write
    mov     rdi, 1                     ; fd = stdout
    lea     rsi, [rbp - 1]             ; buffer
    mov     rdx, 1                     ; length = 1
    syscall

    mov     rsp, rbp
    pop     rbp
    ret
