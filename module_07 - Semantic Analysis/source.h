/*
 * source.h — Module 01 interface (unchanged)
 *
 * Wraps a source file into a struct so every later stage can read
 * the raw text and the filename without re-opening the file.
 */

#ifndef SOURCE_H
#define SOURCE_H

#include <stddef.h>   /* size_t */

/* Source holds the entire contents of one C source file. */
typedef struct {
    const char *filename; /* original path, e.g. "sample.c"         */
    char       *text;     /* null-terminated file contents           */
    size_t      len;      /* number of bytes (excluding the '\0')    */
} Source;

/* Open a file, read it into memory, return a heap-allocated Source.
 * Returns NULL and prints to stderr on failure.                      */
Source *source_open(const char *filename);

/* Release all memory owned by src (including src itself).           */
void source_free(Source *src);

#endif /* SOURCE_H */
