/*
 * source.c — Source file reader implementation
 * Module 11: Code Generation — Control Flow
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "source.h"

int source_open(Source *src, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        return 0;
    }

    /* Determine file size */
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return 0;
    }
    rewind(f);

    /* Read entire file */
    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "error: out of memory\n");
        return 0;
    }
    size_t n = fread(buf, 1, (size_t)size, f);
    buf[n] = '\0';
    fclose(f);

    src->buf  = buf;
    src->path = path;
    src->len  = n;
    return 1;
}

void source_free(Source *src) {
    free(src->buf);
    src->buf = NULL;
    src->len = 0;
}
