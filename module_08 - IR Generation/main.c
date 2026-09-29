/* main.c — IR Generation Demo for my_compiler
 * Module 08: IR Generation
 *
 * Usage: ./ir_gen <source_file.c>
 *
 * Demonstrates the full pipeline:
 *   source → lexer → parser → sema → irgen → ir_print
 */
#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("=== IR Generation Demo — Module 08 ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    /* Step 1: Open and read the source file */
    Source *src = source_open(argv[1]);
    if (!src) return 1;

    /* Step 2: Lex + parse → AST */
    Lexer  lx;
    Parser p;
    lexer_init(&lx, src);
    Node *program = parse_program(&p, &lx);
    if (!program || p.had_error) {
        fprintf(stderr, "parse errors — aborting\n");
        source_free(src);
        return 1;
    }

    /* Step 3: Semantic analysis — check types, scopes, declarations */
    Sema sema;
    if (sema_check(&sema, program)) {
        fprintf(stderr, "semantic errors — aborting\n");
        node_free(program);
        source_free(src);
        return 1;
    }

    /* Step 4: Generate IR from the AST */
    printf("Generating IR for: %s\n\n", argv[1]);
    IRProg *ir = irgen(program);

    /* Step 5: Print the IR in human-readable form */
    ir_print(ir);

    /* Step 6: Clean up all allocated memory */
    ir_free(ir);
    node_free(program);
    source_free(src);

    return 0;
}
