; runtime.asm — my_compiler standard library
; nasm -f elf64 runtime.asm -o runtime.o

section .text
global print_int
global print_newline

; print_int: print integer in rdi to stdout, followed by newline
print_int:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32
    ; save the integer
    mov     rax, rdi
    ; We'll build the string in [rbp-20] to [rbp-1]
    ; and put a newline at [rbp-1]
    mov     byte [rbp-1], 10    ; '\n'
    lea     rcx, [rbp-2]        ; write position, moving left

    ; handle zero
    test    rax, rax
    jnz     .not_zero
    mov     byte [rcx], '0'
    dec     rcx
    jmp     .emit

.not_zero:
    ; handle negative
    xor     r8d, r8d            ; sign = 0
    test    rax, rax
    jns     .digits
    inc     r8d                 ; sign = 1
    neg     rax

.digits:
    mov     r9, 10
.loop:
    xor     rdx, rdx
    div     r9
    add     dl, '0'
    mov     [rcx], dl
    dec     rcx
    test    rax, rax
    jnz     .loop
    test    r8d, r8d
    jz      .emit
    mov     byte [rcx], '-'
    dec     rcx

.emit:
    inc     rcx                 ; first char of the number
    ; length = (rbp-1) - rcx + 1 + 1 (the newline at rbp-1)
    lea     rdx, [rbp-1]
    sub     rdx, rcx
    add     rdx, 2              ; include position rcx itself plus newline
    mov     rsi, rcx
    mov     rax, 1              ; sys_write
    mov     rdi, 1              ; fd = stdout
    syscall
    mov     rsp, rbp
    pop     rbp
    ret

print_newline:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 16
    mov     byte [rbp-1], 10
    mov     rax, 1
    mov     rdi, 1
    lea     rsi, [rbp-1]
    mov     rdx, 1
    syscall
    mov     rsp, rbp
    pop     rbp
    ret
