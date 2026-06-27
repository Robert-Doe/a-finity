/* source.c — Source file abstraction for my_compiler
 * Module 01 — introduced here, unchanged across all modules.
 * Prerequisites: none.
 */
#include "source.h"

#include <stdio.h>    /* fopen, fseek, ftell, fread, fclose, fprintf, perror */
#include <stdlib.h>   /* malloc, exit, free */
#include <string.h>   /* memset */

/* source_open: open the file at 'path', read it entirely into heap memory,
 * and return a Source struct.  Exits the process on any I/O error so callers
 * never have to check for a NULL buffer. */
Source source_open(const char *path) {
    Source src;

    /* Open for reading in binary mode so we get exact byte counts on Windows. */
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror(path);
        exit(1);
    }

    /* Seek to end to discover the file size. */
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek");
        exit(1);
    }
    long size = ftell(fp);
    if (size < 0) {
        perror("ftell");
        exit(1);
    }
    rewind(fp);  /* seek back to the beginning */

    /* Allocate one extra byte for a NUL terminator so the buffer is also a
     * valid C string — handy for string functions in the lexer. */
    src.text = malloc((size_t)size + 1);
    if (!src.text) {
        fprintf(stderr, "out of memory reading %s\n", path);
        exit(1);
    }

    size_t nread = fread(src.text, 1, (size_t)size, fp);
    if ((long)nread != size) {
        fprintf(stderr, "short read on %s\n", path);
        exit(1);
    }
    src.text[size] = '\0';  /* NUL-terminate */
    src.len      = (size_t)size;
    src.filename = path;

    fclose(fp);
    return src;
}

/* source_free: release the text buffer and NULL-out the pointer. */
void source_free(Source *src) {
    free(src->text);
    src->text = NULL;
    src->len  = 0;
}
