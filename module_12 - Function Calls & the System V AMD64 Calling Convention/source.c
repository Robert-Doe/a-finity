/* source.c — Source file reader implementation for my_compiler
 * Module 01: Source handling
 */
#include "source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Open filename, slurp its contents into a new Source struct. */
Source *source_open(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s'\n", filename);
        return NULL;
    }

    /* Seek to end to get file size */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char *text = malloc((size_t)size + 1); /* +1 for null terminator */
    if (!text) {
        fprintf(stderr, "error: out of memory\n");
        fclose(fp);
        return NULL;
    }

    fread(text, 1, (size_t)size, fp);
    text[size] = '\0';
    fclose(fp);

    Source *src = malloc(sizeof(Source));
    if (!src) {
        free(text);
        return NULL;
    }

    src->filename = filename;
    src->text     = text;
    src->len      = (size_t)size;
    src->pos      = 0;
    src->line     = 1;
    src->col      = 1;
    return src;
}

/* Release all memory owned by src. */
void source_free(Source *src) {
    if (!src) return;
    free(src->text);
    free(src);
}
