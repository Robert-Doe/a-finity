/*
 * source.c — Source file management for mycc
 * Module 16: The Complete Compiler
 */
#include "source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int source_open(Source *src, const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "mycc: cannot open '%s'\n", filename);
        return -1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "mycc: cannot seek '%s'\n", filename);
        fclose(f);
        return -1;
    }
    long len = ftell(f);
    if (len < 0) {
        fprintf(stderr, "mycc: cannot tell '%s'\n", filename);
        fclose(f);
        return -1;
    }
    rewind(f);

    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fprintf(stderr, "mycc: out of memory reading '%s'\n", filename);
        fclose(f);
        return -1;
    }

    size_t nread = fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[nread] = '\0';

    src->filename = filename;
    src->text     = buf;
    src->length   = nread;
    return 0;
}

void source_close(Source *src)
{
    free(src->text);
    src->text   = NULL;
    src->length = 0;
}
