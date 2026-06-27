/* codegen.h — Code generator for my_compiler (ELF binary output)
 * Module 14: ELF Object File Emitter
 *
 * Translates IR to raw x86-64 machine code via the x86enc layer.
 * The output is a byte array per function, collected into an ElfEmitter.
 */
#ifndef MY_COMPILER_CODEGEN_H
#define MY_COMPILER_CODEGEN_H

#include "ir.h"
#include "regalloc.h"
#include "elf_emit.h"

/* Generate machine code for all functions in prog and add them to e. */
void codegen_program(const IRProg *prog, ElfEmitter *e);

#endif /* MY_COMPILER_CODEGEN_H */
