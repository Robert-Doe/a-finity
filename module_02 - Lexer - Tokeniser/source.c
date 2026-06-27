/*
 * source.c — Implementation of Source file I/O
 * Module 01 (copied unchanged into Module 02)
 *
 * Reads an entire source file into a heap buffer so the rest of
 * the compiler can work with plain pointers into a contiguous array.
 */

#include "source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * source_open — open a file and read it entirely into memory.
 *
 * path: filesystem path to the source file.
 * Returns: a Source struct with text pointing to a null-terminated
 *          heap buffer and len set to the file byte count.
 * Assumes: path is a valid, readable file.
 * Note: calls exit(1) on any I/O or allocation failure so callers
 *       don't need to check for NULL.
 */
Source source_open(const char *path)
{
    Source src;
    src.filename = path;  /* keep a reference to the path for error messages */

    FILE *fp = fopen(path, "rb");  /* binary mode so ftell gives exact byte count */
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        exit(1);
    }

    /* determine file size by seeking to the end */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: cannot seek in '%s'\n", path);
        exit(1);
    }
    long size = ftell(fp);  /* byte offset at end == file length */
    if (size < 0) {
        fprintf(stderr, "error: cannot ftell '%s'\n", path);
        exit(1);
    }
    rewind(fp);  /* go back to the beginning before reading */

    /* allocate one extra byte for the null terminator */
    src.text = (char *)malloc((size_t)size + 1);
    if (!src.text) {
        fprintf(stderr, "error: out of memory reading '%s'\n", path);
        exit(1);
    }

    /* read the entire file in one call */
    size_t nread = fread(src.text, 1, (size_t)size, fp);
    if ((long)nread != size) {
        fprintf(stderr, "error: short read on '%s'\n", path);
        exit(1);
    }

    src.text[size] = '\0';  /* null-terminate so string functions work on the buffer */
    src.len = (size_t)size;

    fclose(fp);
    return src;
}

/*
 * source_free — release memory owned by a Source.
 *
 * src: pointer to a Source previously returned by source_open.
 * Note: sets text to NULL after freeing to catch use-after-free bugs.
 */
void source_free(Source *src)
{
    free(src->text);   /* free the heap buffer allocated in source_open */
    src->text = NULL;  /* poison the pointer so double-free is obvious */
    src->len  = 0;
}
