/* main.c — Full compiler pipeline for bob_compiler Module 13
 *
 * Usage: ./regalloc <source.c> [-o output.asm]
 *
 * Pipeline:
 *   source → lexer → parser → sema → irgen → opt → regalloc → codegen
 *
 * Prints the IR and the register assignment before emitting assembly.
 */
#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"
#include "regalloc.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("=== bob_compiler — Module 13: Register Allocation ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "usage: %s <source.c> [-o output.asm]\n", argv[0]);
        return 1;
    }

    const char *src_file = argv[1];
    const char *out_file = NULL;

    /* Parse command-line flags */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            out_file = argv[++i];
    }

    /* ---------------------------------------------------------------- */
    /* Step 1: Open source                                               */
    /* ---------------------------------------------------------------- */
    Source *src = source_open(src_file);
    if (!src) return 1;
    printf("[1] Source: %s\n\n", src_file);

    /* ---------------------------------------------------------------- */
    /* Step 2: Lex + parse                                               */
    /* ---------------------------------------------------------------- */
    Lexer  lx;
    Parser p;
    lexer_init(&lx, src);
    parser_init(&p, &lx);
    Node *program = parse_program(&p);
    if (!program || p.had_error) {
        fprintf(stderr, "parse errors — aborting\n");
        source_free(src);
        return 1;
    }
    printf("[2] Parse: OK\n\n");

    /* ---------------------------------------------------------------- */
    /* Step 3: Semantic analysis                                         */
    /* ---------------------------------------------------------------- */
    if (sema_check(program) != 0) {
        fprintf(stderr, "semantic errors — aborting\n");
        node_free(program);
        source_free(src);
        return 1;
    }
    printf("[3] Sema: OK\n\n");

    /* ---------------------------------------------------------------- */
    /* Step 4: IR generation                                             */
    /* ---------------------------------------------------------------- */
    IRProg *ir = irgen(program);
    printf("[4] IR generation: OK\n\n");
    ir_print(ir);

    /* ---------------------------------------------------------------- */
    /* Step 5: Optimisation (constant folding)                           */
    /* ---------------------------------------------------------------- */
    opt_run(ir);
    printf("[5] Optimisation: OK\n\n");

    /* ---------------------------------------------------------------- */
    /* Step 6: Code generation (with register allocation inside)        */
    /* ---------------------------------------------------------------- */
    FILE *out_fp = stdout;
    if (out_file) {
        out_fp = fopen(out_file, "w");
        if (!out_fp) {
            fprintf(stderr, "error: cannot open '%s' for writing\n", out_file);
            ir_free(ir);
            node_free(program);
            source_free(src);
            return 1;
        }
    }

    printf("[6] Register allocation + code generation:\n\n");
    codegen(ir, out_fp);

    if (out_file) {
        fclose(out_fp);
        printf("\n[6] Assembly written to: %s\n", out_file);
    }

    /* ---------------------------------------------------------------- */
    /* Clean up                                                          */
    /* ---------------------------------------------------------------- */
    ir_free(ir);
    node_free(program);
    source_free(src);

    printf("\nDone.\n");
    return 0;
}
