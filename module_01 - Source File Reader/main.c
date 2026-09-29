/*
 * main.c — Demo driver for Module 01: Source file reader
 *
 * Opens the file named on the command line, prints its byte count,
 * line count, and a short character preview to stdout.  This is the
 * visible proof that source_open() works before we build the lexer.
 *
 * Module introduced : 01 — Source file reader
 * Previous equivalent: none
 * Prerequisites     : source.h, source.c
 */

#include <stdio.h>    /* printf, putchar, fprintf */
#include <stdlib.h>   /* exit                     */

#include "source.h"

/* Maximum characters to show in the preview.  80 fits one terminal line
 * and is enough to confirm the file was read correctly.                  */
#define PREVIEW_LEN 80

/*
 * count_lines — count newline characters in a Source buffer.
 *
 * src    : a valid Source returned by source_open; must not be NULL.
 * returns: number of '\n' bytes found, equal to the line count for files
 *          that end with a newline (virtually all well-formed C files do).
 *
 * Assumes: src->text is not NULL and src->len is accurate.
 */
static size_t count_lines(const Source *src)
{
    size_t i;
    size_t lines = 0;

    for (i = 0; i < src->len; i++) {
        if (src->text[i] == '\n')
            lines++;   /* each '\n' terminates one line */
    }
    return lines;
}

/*
 * print_preview — print up to PREVIEW_LEN characters of src to stdout.
 *
 * src    : a valid Source whose text to preview; must not be NULL.
 * returns: nothing; output goes to stdout.
 *
 * Assumes: src->text is not NULL.
 * Note   : newlines are rendered as the two-char sequence \n so the
 *          preview stays on a single terminal line.
 */
static void print_preview(const Source *src)
{
    size_t i;
    /* Clamp to whichever is smaller: the actual file or PREVIEW_LEN. */
    size_t limit = src->len < PREVIEW_LEN ? src->len : PREVIEW_LEN;

    for (i = 0; i < limit; i++) {
        if (src->text[i] == '\n') {
            /* Escape newlines so the preview stays on one visible line. */
            putchar('\\');
            putchar('n');
        } else {
            putchar((unsigned char)src->text[i]);
        }
    }

    /* Signal truncation if there is more content beyond the preview. */
    if (src->len > PREVIEW_LEN)
        printf(" ...");

    putchar('\n');
}

int main(int argc, char **argv)
{
    Source src;

    /* Require exactly one argument: the path to the source file. */
    if (argc != 2) {
        fprintf(stderr, "usage: reader <file.c>\n");
        return 1;   /* non-zero exit code signals failure to the shell */
    }

    src = source_open(argv[1]);   /* load entire file into a heap buffer */

    printf("=== Source File Reader - Module 01 ===\n");
    printf("file    : %s\n",   src.filename);
    printf("size    : %zu bytes\n", src.len);
    printf("lines   : %zu\n",  count_lines(&src));
    printf("preview : ");
    print_preview(&src);

    source_free(&src);   /* always free before exit; builds the habit */
    return 0;
}
