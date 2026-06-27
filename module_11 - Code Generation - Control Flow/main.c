/*
 * main.c — Compiler driver for Module 11
 * Module 11: Code Generation — Control Flow
 *
 * Pipeline:
 *   source file  →  [lex]  →  tokens
 *   tokens       →  [parse]  →  AST
 *   AST          →  [sema]   →  checked AST
 *   AST          →  [irgen]  →  IR
 *   IR           →  [opt]    →  optimized IR
 *   IR           →  [codegen]→  NASM assembly
 *
 * Usage:
 *   ./codegen11 input.c [-o output.asm]
 *
 * If -o is omitted, assembly is written to stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"
#include "codegen.h"

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <input.c> [-o <output.asm>]\n", prog);
    fprintf(stderr, "  Compiles a subset-C program to x86-64 NASM assembly.\n");
    fprintf(stderr, "  If -o is omitted, assembly is printed to stdout.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *input_path  = NULL;
    const char *output_path = NULL;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: -o requires a filename\n");
                return 1;
            }
            output_path = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown flag '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        } else {
            if (input_path) {
                fprintf(stderr, "error: multiple input files not supported\n");
                return 1;
            }
            input_path = argv[i];
        }
    }

    if (!input_path) {
        fprintf(stderr, "error: no input file\n");
        usage(argv[0]);
        return 1;
    }

    /* ── 1. Read source ─────────────────────────────────────────────── */
    Source src;
    if (!source_open(&src, input_path)) return 1;

    /* ── 2. Parse ───────────────────────────────────────────────────── */
    Parser parser;
    parser_init(&parser, src.buf);
    Node *ast = parser_parse(&parser);

    if (parser.errors) {
        fprintf(stderr, "parse failed with %d error(s)\n", parser.errors);
        ast_free(ast);
        source_free(&src);
        return 1;
    }

    /* ── 3. Semantic analysis ───────────────────────────────────────── */
    Sema sema;
    sema_init(&sema);
    if (!sema_check(&sema, ast)) {
        fprintf(stderr, "sema failed with %d error(s)\n", sema.errors);
        sema_free(&sema);
        ast_free(ast);
        source_free(&src);
        return 1;
    }
    sema_free(&sema);

    /* ── 4. IR generation ───────────────────────────────────────────── */
    IRProg ir;
    if (!ir_gen(&ir, ast)) {
        fprintf(stderr, "IR generation failed\n");
        ast_free(ast);
        source_free(&src);
        return 1;
    }
    ast_free(ast);
    source_free(&src);

    /* ── 5. Optimization ────────────────────────────────────────────── */
    opt_run(&ir);

    /* ── 6. Code generation ─────────────────────────────────────────── */
    FILE *out = stdout;
    if (output_path) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "error: cannot open output '%s'\n", output_path);
            ir_free(&ir);
            return 1;
        }
    }

    codegen_emit(&ir, out);

    if (output_path) {
        fclose(out);
        fprintf(stderr, "Assembly written to '%s'\n", output_path);
    }

    ir_free(&ir);
    return 0;
}
