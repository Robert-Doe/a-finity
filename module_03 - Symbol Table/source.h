/*
 * source.h — Source file abstraction
 * Module 01 (copied unchanged into Module 03)
 *
 * Provides the Source struct and functions to open/free a source file.
 */

#ifndef MY_COMPILER_SOURCE_H
#define MY_COMPILER_SOURCE_H

#include <stddef.h>

/* Source — holds the entire contents of one source file in memory */
typedef struct {
    char       *text;      /* null-terminated buffer of the file contents */
    size_t      len;       /* number of bytes in text (excluding null terminator) */
    const char *filename;  /* original path passed to source_open */
} Source;

/*
 * source_open — read an entire file into a Source struct.
 * path: filesystem path to the source file.
 * Returns a Source with text != NULL on success; exits on failure.
 */
Source source_open(const char *path);

/*
 * source_free — release the memory held by a Source.
 * src: pointer to Source previously initialised by source_open.
 */
void source_free(Source *src);

#endif /* MY_COMPILER_SOURCE_H */
