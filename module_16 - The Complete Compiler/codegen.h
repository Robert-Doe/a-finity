/*
 * codegen.h — NASM x86-64 code generator interface for mycc
 * Module 16: The Complete Compiler
 */
#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"
#include <stdio.h>

/*
 * Emit NASM assembly for the entire IR program to the given file.
 * Returns 0 on success, -1 on error.
 */
int codegen(const IRProgram *prog, FILE *out);

#endif /* CODEGEN_H */
