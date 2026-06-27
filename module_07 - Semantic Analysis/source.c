/*
 * source.c — Module 01 implementation (unchanged)
 *
 * Opens a file, slurps it into a malloc'd buffer, and hands back a
 * Source struct for all later compiler stages.
 */

#include "source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Source *source_open(const char *filename)
{
    /* --- open the file ------------------------------------------- */
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open '%s'\n", filename);
        return NULL;
    }

    /* --- find file size ------------------------------------------ */
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: fseek failed on '%s'\n", filename);
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fprintf(stderr, "error: ftell failed on '%s'\n", filename);
        fclose(f);
        return NULL;
    }
    rewind(f);

    /* --- allocate buffer ----------------------------------------- */
    char *text = malloc((size_t)size + 1);
    if (!text) {
        fprintf(stderr, "error: out of memory reading '%s'\n", filename);
        fclose(f);
        return NULL;
    }

    /* --- read contents ------------------------------------------- */
    size_t nread = fread(text, 1, (size_t)size, f);
    fclose(f);
    text[nread] = '\0';   /* null-terminate regardless of nread       */

    /* --- build the Source struct --------------------------------- */
    Source *src = malloc(sizeof(Source));
    if (!src) {
        fprintf(stderr, "error: out of memory\n");
        free(text);
        return NULL;
    }
    src->filename = filename;   /* borrow the caller's string        */
    src->text     = text;
    src->len      = nread;
    return src;
}

void source_free(Source *src)
{
    if (!src) return;
    free(src->text);
    free(src);
}
