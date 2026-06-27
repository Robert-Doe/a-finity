/*
 * source.h — Source file management for mycc
 * Module 16: The Complete Compiler
 */
#ifndef SOURCE_H
#define SOURCE_H

#include <stddef.h>

typedef struct {
    const char *filename;
    char       *text;      /* entire file contents, NUL-terminated */
    size_t      length;
} Source;

/* Open and read the entire file into a Source.
   Returns 0 on success, -1 on error (message printed to stderr). */
int source_open(Source *src, const char *filename);

/* Free the text buffer allocated by source_open. */
void source_close(Source *src);

#endif /* SOURCE_H */
