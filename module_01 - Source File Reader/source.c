/*
 * source.c — Implementation of the source file reader
 *
 * Reads an entire source file into a single heap-allocated buffer using
 * the measure-allocate-read-terminate pattern.  This is the first stage
 * of the my_compiler pipeline: before any token can be recognised, the
 * raw bytes must be in memory with a known length.
 *
 * Module introduced : 01 — Source file reader
 * Previous equivalent: none
 * Prerequisites     : fopen/fclose, fseek/ftell, fread, malloc/free, exit
 */

#include <stdio.h>    /* fopen, fclose, fseek, ftell, fread, fprintf        */
#include <stdlib.h>   /* malloc, free, exit                                  */

#include "source.h"

/* ------------------------------------------------------------------ */
/* Internal helper                                                      */
/* ------------------------------------------------------------------ */

/*
 * file_byte_count — seek to the end of fp and return the file size.
 *
 * fp     : an open FILE* in binary mode; may be at any position.
 * returns: number of bytes in the file, or -1L on any seek error.
 *
 * Assumes: fp was opened with "rb" so SEEK_END is meaningful.
 * Note   : rewinds fp to byte 0 before returning so the caller can fread.
 */
static long file_byte_count(FILE *fp)
{
    long size;

    /* Move the file position indicator to the very end. */
    if (fseek(fp, 0, SEEK_END) != 0)
        return -1L;   /* seek failed; caller checks for negative return */

    size = ftell(fp);  /* distance from start = total byte count */
    if (size < 0)
        return -1L;   /* ftell returns -1 on error */

    /* Rewind to byte 0 so the caller's fread starts from the beginning. */
    if (fseek(fp, 0, SEEK_SET) != 0)
        return -1L;

    return size;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

/*
 * source_open — open a file and read its entire contents into memory.
 *
 * path   : path to the source file; must not be NULL.
 * returns: a Source with .text heap-allocated and null-terminated.
 *          Calls exit(1) on error; the caller never receives a bad Source.
 *
 * Assumes: file fits in available RAM.
 * Note   : caller must call source_free() when done with the Source.
 */
Source source_open(const char *path)
{
    Source src;
    FILE  *fp;
    long   size;
    size_t bytes_read;

    /* Open in binary mode: "rb" prevents Windows from silently converting
     * \r\n pairs to \n, which would corrupt byte offsets used in error
     * messages.  The lexer (Module 02) handles line endings explicitly.  */
    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        exit(1);   /* no source file = no compiler run; stop cleanly */
    }

    size = file_byte_count(fp);   /* measure before allocating */
    if (size < 0) {
        fprintf(stderr, "error: cannot determine size of '%s'\n", path);
        fclose(fp);
        exit(1);   /* seek failure is unexpected and unrecoverable here */
    }

    /* Allocate size+1 bytes: the +1 holds the null terminator we append
     * after reading so that C string functions work on the buffer.       */
    src.text = malloc((size_t)size + 1);
    if (src.text == NULL) {
        fprintf(stderr, "error: out of memory reading '%s'\n", path);
        fclose(fp);
        exit(1);   /* malloc returned NULL; we cannot proceed without RAM */
    }

    /* Read the entire file in one call.  fread returns the number of
     * items successfully read; we asked for 'size' items of 1 byte each,
     * so a correct read returns exactly (size_t)size.                    */
    bytes_read = fread(src.text, 1, (size_t)size, fp);
    if ((long)bytes_read != size) {
        /* Partial read: could be a race condition (file truncated between
         * ftell and fread) or a read error.  Either way, abort.          */
        fprintf(stderr,
                "error: partial read of '%s' (%zu of %ld bytes)\n",
                path, bytes_read, size);
        free(src.text);   /* we touched this; caller never saw it, so free here */
        fclose(fp);
        exit(1);
    }

    src.text[size] = '\0';   /* null-terminate so string functions are safe */
    src.len        = (size_t)size;
    src.filename   = path;   /* borrow caller's pointer; caller owns the string */

    fclose(fp);   /* done reading; release the OS file handle immediately */
    return src;
}

/*
 * source_free — release heap memory owned by a Source.
 *
 * src    : pointer to a Source returned by source_open; must not be NULL.
 * returns: nothing.
 *
 * Assumes: src->text was allocated by source_open.
 * Note   : poisons src->text with NULL so dangling-pointer bugs crash
 *          immediately at the bad access rather than silently corrupting data.
 */
void source_free(Source *src)
{
    free(src->text);    /* release the buffer malloc'd in source_open */
    src->text = NULL;   /* pointer poisoning: any use-after-free will segfault */
    src->len  = 0;      /* make the struct obviously invalid to future readers */
}
