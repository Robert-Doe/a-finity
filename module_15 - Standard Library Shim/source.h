/* source.h — Source file reader for my_compiler
 * Module 01: Source handling
 * No prerequisites.
 */
#ifndef MY_COMPILER_SOURCE_H
#define MY_COMPILER_SOURCE_H

#include <stddef.h>

/* Holds the entire source text and a cursor for the lexer. */
typedef struct {
    const char *filename; /* path that was opened */
    char       *text;     /* full file contents (null-terminated) */
    size_t      len;      /* length of text in bytes */
    size_t      pos;      /* current read position */
    int         line;     /* 1-based current line number */
    int         col;      /* 1-based current column number */
} Source;

/* Open filename, read all bytes into a heap-allocated Source.
 * Returns NULL and prints an error on failure. */
Source *source_open(const char *filename);

/* Release all memory owned by src. */
void source_free(Source *src);

#endif /* MY_COMPILER_SOURCE_H */
