/*
 * source.h — Source file reader
 *
 * Reads an entire C source file into memory so the lexer can work on it.
 * Module 11: Code Generation — Control Flow
 */

#ifndef SOURCE_H
#define SOURCE_H

typedef struct {
    char       *buf;   /* null-terminated file contents */
    const char *path;  /* original file path (for error messages) */
    size_t      len;   /* number of bytes (excluding null terminator) */
} Source;

/* Read the file at 'path' into a Source. Returns 1 on success, 0 on error. */
int source_open(Source *src, const char *path);

/* Free memory allocated by source_open. */
void source_free(Source *src);

#endif /* SOURCE_H */
