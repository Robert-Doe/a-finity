/*
 * source.h — Source file abstraction for my_compiler
 *
 * Declares the Source struct and the two functions that open and free
 * a source file.  Every other module in my_compiler receives a Source*
 * or Source value — this is the first data structure in the pipeline.
 *
 * Module introduced : 01 — Source file reader
 * Previous equivalent: none (this is the first module)
 * Prerequisites     : basic C structs, malloc/free, FILE* and fopen/fclose
 */
#ifndef MY_COMPILER_SOURCE_H
#define MY_COMPILER_SOURCE_H

#include <stddef.h>   /* size_t — the correct unsigned type for memory sizes */

/* ------------------------------------------------------------------
 * Source — holds the entire text of one source file in memory.
 * All later modules receive a const Source* and read from it.
 * ------------------------------------------------------------------ */
typedef struct {
    char       *text;      /* heap-allocated, null-terminated file contents  */
    size_t      len;       /* byte count — always use this, NEVER strlen()   */
    const char *filename;  /* original path string, used in error messages   */
} Source;

/*
 * source_open — open a source file and read its entire contents into memory.
 *
 * path   : path to the file to open; must not be NULL
 * returns: a Source whose .text is heap-allocated and null-terminated.
 *          On any error (file not found, out of memory, partial read),
 *          prints a message to stderr and calls exit(1).
 *
 * Assumes: the file fits in available RAM (no streaming in Module 01).
 * Note   : caller must eventually call source_free() on the returned value.
 */
Source source_open(const char *path);

/*
 * source_free — release all heap memory owned by a Source.
 *
 * src    : pointer to a Source previously returned by source_open; not NULL.
 * returns: nothing.
 *
 * Assumes: src->text was allocated by source_open, not a string literal.
 * Note   : sets src->text to NULL after freeing to catch use-after-free.
 */
void source_free(Source *src);

#endif /* MY_COMPILER_SOURCE_H */
