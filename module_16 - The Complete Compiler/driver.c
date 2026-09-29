/*
 * driver.c — Main compiler driver for mycc
 * Module 16: The Complete Compiler
 *
 * Usage: mycc <input.c> [-o <output>]
 *
 * Pipeline:
 *   source_open → lex → parse → sema → irgen → opt → codegen
 *   → nasm -f elf64 → gcc -no-pie → binary
 */
#include "source.h"
#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#  define PATH_SEP '\\'
#else
#  define PATH_SEP '/'
#endif

/* ----------------------------------------------------------------- timing */

typedef struct {
    const char *phase;
    double      ms;
} TimingEntry;

static double clock_ms(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC * 1000.0;
}

/* ----------------------------------------------------------------- helpers */

/* Derive output path from input: strip ".c" suffix */
static void derive_output(const char *input, char *out, size_t cap)
{
    size_t len = strlen(input);
    if (len > 2 && input[len-2] == '.' && input[len-1] == 'c') {
        size_t copy = len - 2;
        if (copy >= cap) copy = cap - 1;
        memcpy(out, input, copy);
        out[copy] = '\0';
    } else {
        snprintf(out, cap, "%s.out", input);
    }
}

/* Find the directory containing the running executable (argv[0]). */
static void exe_dir(const char *argv0, char *dir, size_t cap)
{
    strncpy(dir, argv0, cap - 1);
    dir[cap - 1] = '\0';
    /* Find last path separator */
    char *sep = strrchr(dir, PATH_SEP);
#ifndef _WIN32
    char *sep2 = strrchr(dir, '/');
    if (!sep || (sep2 && sep2 > sep)) sep = sep2;
#endif
    if (sep) { sep[1] = '\0'; }
    else      { dir[0] = '.'; dir[1] = PATH_SEP; dir[2] = '\0'; }
}

static void print_timing_table(TimingEntry *entries, int n)
{
    printf("\n%-20s %10s\n", "Phase", "Time (ms)");
    printf("%-20s %10s\n", "--------------------", "----------");
    double total = 0;
    for (int i = 0; i < n; i++) {
        printf("%-20s %10.3f\n", entries[i].phase, entries[i].ms);
        total += entries[i].ms;
    }
    printf("%-20s %10.3f\n", "TOTAL", total);
    printf("\n");
}

/* ----------------------------------------------------------------- main */

int main(int argc, char *argv[])
{
    const char *input_file  = NULL;
    const char *output_file = NULL;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "mycc: unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!input_file) {
        fprintf(stderr, "usage: mycc <input.c> [-o <output>]\n");
        return 1;
    }

    /* Derive output name if not given */
    char out_buf[512];
    if (!output_file) {
        derive_output(input_file, out_buf, sizeof out_buf);
        output_file = out_buf;
    }

    TimingEntry timing[16];
    int         ntiming = 0;
    double      t0, t1;

    /* ---- Phase 1: source ---- */
    t0 = clock_ms();
    Source src;
    if (source_open(&src, input_file) != 0) return 1;
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "source_open", t1 - t0 };

    /* ---- Phase 2: lex ---- */
    t0 = clock_ms();
    Lexer lex;
    lexer_init(&lex, &src);
    /* Lex all tokens to count them (warm the lexer) */
    {
        Lexer tmp = lex;
        int tok_count = 0;
        Token t;
        do { t = lexer_next(&tmp); tok_count++; } while (t.type != TOK_EOF);
        /* Reset for parser */
        lexer_init(&lex, &src);
    }
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "lex", t1 - t0 };

    /* ---- Phase 3: parse ---- */
    t0 = clock_ms();
    Parser parser;
    parser_init(&parser, &lex);
    ASTNode *program = parse_program(&parser);
    if (parser.errors > 0) {
        fprintf(stderr, "mycc: %d parse error(s). Aborting.\n", parser.errors);
        ast_free(program);
        source_close(&src);
        return 1;
    }
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "parse", t1 - t0 };

    /* ---- Phase 4: sema ---- */
    t0 = clock_ms();
    SemaCtx sema;
    sema_ctx_init(&sema);
    int sema_errs = sema_check(&sema, program);
    sema_ctx_free(&sema);
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "sema", t1 - t0 };
    if (sema_errs > 0) {
        fprintf(stderr, "mycc: %d semantic error(s). Aborting.\n", sema_errs);
        ast_free(program);
        source_close(&src);
        return 1;
    }

    /* ---- Phase 5: irgen ---- */
    t0 = clock_ms();
    IRProgram *ir = irgen(program);
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "irgen", t1 - t0 };
    ast_free(program);

    /* ---- Phase 6: opt ---- */
    t0 = clock_ms();
    int n_elim = opt_all(ir);
    (void)n_elim;
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "opt", t1 - t0 };

    /* ---- Phase 7: codegen to temp .asm ---- */
    char tmp_asm[512];
    char tmp_obj[512];
    snprintf(tmp_asm, sizeof tmp_asm, "/tmp/mycc_%d.asm", (int)clock());
    snprintf(tmp_obj, sizeof tmp_obj, "/tmp/mycc_%d.o",   (int)clock());

    t0 = clock_ms();
    FILE *asm_fp = fopen(tmp_asm, "w");
    if (!asm_fp) {
        fprintf(stderr, "mycc: cannot write temp asm '%s'\n", tmp_asm);
        ir_program_free(ir);
        source_close(&src);
        return 1;
    }
    codegen(ir, asm_fp);
    fclose(asm_fp);
    ir_program_free(ir);
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "codegen", t1 - t0 };

    source_close(&src);

    /* ---- Phase 8: nasm ---- */
    char cmd[1024];
    snprintf(cmd, sizeof cmd, "nasm -f elf64 %s -o %s", tmp_asm, tmp_obj);
    t0 = clock_ms();
    int rc = system(cmd);
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "nasm", t1 - t0 };
    if (rc != 0) {
        fprintf(stderr, "mycc: nasm failed (exit %d)\n", rc);
        remove(tmp_asm);
        return 1;
    }
    remove(tmp_asm);

    /* ---- Phase 9: find runtime.o ---- */
    char runtime_path[512];
    char exe_directory[512];
    exe_dir(argv[0], exe_directory, sizeof exe_directory);
    snprintf(runtime_path, sizeof runtime_path, "%sruntime.o", exe_directory);

    FILE *test_rt = fopen(runtime_path, "rb");
    if (!test_rt) {
        /* Fallback to cwd */
        strncpy(runtime_path, "./runtime.o", sizeof runtime_path - 1);
    } else {
        fclose(test_rt);
    }

    /* ---- Phase 10: link with gcc ---- */
    snprintf(cmd, sizeof cmd, "gcc -no-pie %s %s -o %s",
             tmp_obj, runtime_path, output_file);
    t0 = clock_ms();
    rc = system(cmd);
    t1 = clock_ms();
    timing[ntiming++] = (TimingEntry){ "link (gcc)", t1 - t0 };
    remove(tmp_obj);

    if (rc != 0) {
        fprintf(stderr, "mycc: linking failed (exit %d)\n", rc);
        return 1;
    }

    /* ---- Summary ---- */
    print_timing_table(timing, ntiming);
    printf("compiled successfully -> %s\n", output_file);

    return 0;
}
