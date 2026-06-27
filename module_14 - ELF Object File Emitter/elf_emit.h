/* elf_emit.h — ELF64 relocatable object file emitter for my_compiler
 * Module 14: ELF Object File Emitter
 */
#ifndef MY_COMPILER_ELF_EMIT_H
#define MY_COMPILER_ELF_EMIT_H

#include <stdint.h>
#include <stddef.h>

#define ELF_BUF_SIZE (1 << 20)  /* 1 MB */

typedef struct {
    uint8_t  buf[ELF_BUF_SIZE];  /* output buffer for the complete .o file */
    size_t   pos;
    /* .text section */
    uint8_t  text[ELF_BUF_SIZE];
    size_t   text_size;
    /* symbol table: up to 64 functions */
    char     sym_names[64][64];
    size_t   sym_offsets[64];  /* byte offset of each symbol in .text */
    size_t   sym_sizes[64];    /* byte size of each function */
    int      n_syms;
} ElfEmitter;

void elf_init(ElfEmitter *e);
/* Add one function's machine code to the .text section and record its symbol */
void elf_add_func(ElfEmitter *e, const char *name, const uint8_t *code, size_t code_len);
/* Write a complete ELF64 relocatable object file to the given path */
void elf_write(ElfEmitter *e, const char *path);

#endif /* MY_COMPILER_ELF_EMIT_H */
