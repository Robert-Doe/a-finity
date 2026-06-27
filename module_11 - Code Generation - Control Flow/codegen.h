/*
 * codegen.h — x86-64 NASM code generator interface
 * Module 11: Code Generation — Control Flow
 *
 * Converts the optimized IR into NASM assembly.
 * This module handles the full instruction set including:
 *   - Stack frame setup and teardown
 *   - Arithmetic and comparison instructions
 *   - Control flow: IR_LABEL, IR_JUMP, IR_JUMPZ
 *   - Function calls and the System V AMD64 calling convention
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ir.h"

/* Emit NASM assembly for the entire program to 'out'. */
void codegen_emit(const IRProg *prog, FILE *out);

#endif /* CODEGEN_H */
