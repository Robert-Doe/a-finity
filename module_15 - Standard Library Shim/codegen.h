/* codegen.h — NASM x86-64 text code generator for my_compiler
 * Module 15: Standard Library Shim
 * Prerequisites: ir.h
 *
 * Generates NASM-syntax x86-64 assembly (.asm) that is assembled with:
 *   nasm -f elf64 output.asm -o output.o
 *   nasm -f elf64 runtime.asm -o runtime.o
 *   gcc -no-pie output.o runtime.o -o program
 *
 * The runtime.asm file provides:
 *   print_int(rdi)    — convert integer to decimal string, write to stdout
 *   print_newline()   — write a newline to stdout
 *
 * IR_PRINT is lowered to:
 *   mov  rdi, [rbp - offset(src1)]
 *   call print_int
 */
#ifndef MY_COMPILER_CODEGEN_H
#define MY_COMPILER_CODEGEN_H

#include "ir.h"
#include <stdio.h>

/* Generate a complete NASM x86-64 assembly file for prog into out.
 * Emits "extern print_int" and "extern print_newline" declarations at the top. */
void codegen(const IRProg *prog, FILE *out);

#endif /* MY_COMPILER_CODEGEN_H */
