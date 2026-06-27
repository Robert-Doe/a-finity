/* main.c — IR Optimization Demo for my_compiler
 * Module 09: IR Optimization
 *
 * Usage: ./optimize <source_file.c>
 *
 * Runs the full pipeline:
 *   source → lexer → parser → sema → irgen → print BEFORE → opt_all → print AFTER
 */
#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("=== IR Optimization Demo — Module 09 ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    /* ---------------------------------------------------------------- */
    /* Step 1: Open and read the source file                             */
    /* ---------------------------------------------------------------- */
    Source *src = source_open(argv[1]);
    if (!src) return 1;

    /* ---------------------------------------------------------------- */
    /* Step 2: Lex + parse → AST                                         */
    /* ---------------------------------------------------------------- */
    Lexer  lx;
    Parser p;
    lexer_init(&lx, src);
    Node *program = parse_program(&p, &lx);
    if (!program || p.had_error) {
        fprintf(stderr, "parse errors — aborting\n");
        source_free(src);
        return 1;
    }

    /* ---------------------------------------------------------------- */
    /* Step 3: Semantic analysis                                          */
    /* ---------------------------------------------------------------- */
    Sema sema;
    if (sema_check(&sema, program)) {
        fprintf(stderr, "semantic errors — aborting\n");
        node_free(program);
        source_free(src);
        return 1;
    }

    /* ---------------------------------------------------------------- */
    /* Step 4: Generate IR from the AST                                  */
    /* ---------------------------------------------------------------- */
    IRProg *ir = irgen(program);

    /* ---------------------------------------------------------------- */
    /* Step 5: Print the IR BEFORE optimization                          */
    /* ---------------------------------------------------------------- */
    printf("=== Before Optimization ===\n\n");
    ir_print(ir);

    /* ---------------------------------------------------------------- */
    /* Step 6: Run all optimization passes                               */
    /* ---------------------------------------------------------------- */
    opt_all(ir);

    /* ---------------------------------------------------------------- */
    /* Step 7: Print the IR AFTER optimization                           */
    /* ---------------------------------------------------------------- */
    printf("=== After Optimization ===\n\n");
    ir_print(ir);

    /* ---------------------------------------------------------------- */
    /* Step 8: Clean up                                                  */
    /* ---------------------------------------------------------------- */
    ir_free(ir);
    node_free(program);
    source_free(src);

    return 0;
}
