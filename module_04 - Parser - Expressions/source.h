/* source.h — Source file abstraction for my_compiler
 * Module 01 — introduced here, unchanged across all modules.
 * Prerequisites: none.
 */
#ifndef MY_COMPILER_SOURCE_H
#define MY_COMPILER_SOURCE_H

#include <stddef.h>   /* size_t */

/* Source holds the entire contents of one input file in memory. */
typedef struct {
    char       *text;      /* heap-allocated buffer containing file text */
    size_t      len;       /* number of bytes in text (not including NUL) */
    const char *filename;  /* original file path, used in error messages */
} Source;

/* source_open: read the entire file at 'path' into a Source.
 * Returns a Source with text!=NULL on success.
 * On failure, prints an error message and exits. */
Source source_open(const char *path);

/* source_free: release the heap memory held by *src.
 * Sets src->text to NULL to prevent double-free. */
void source_free(Source *src);

#endif /* MY_COMPILER_SOURCE_H */
