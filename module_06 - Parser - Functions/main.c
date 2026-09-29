/* main.c — Module 06 driver: parse a full C-like file and print the AST.
 * Module 06 — Function parser demo.
 * Prerequisites: source.h, parser.h, ast.h.
 *
 * Usage: ./parser_func <source-file>
 *   Reads the source file, parses all function definitions,
 *   prints the complete AST with node_print, and reports any errors.
 */
#include <stdio.h>    /* printf, fprintf */
#include <stdlib.h>   /* exit */

#include "source.h"
#include "parser.h"
#include "ast.h"

int main(int argc, char *argv[]) {
    /* Require exactly one argument: the source file path. */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        fprintf(stderr, "  Parses the file and prints the AST.\n");
        return 1;
    }

    /* --- Load the source file into memory --- */
    Source src = source_open(argv[1]);  /* exits on failure */

    /* --- Initialise the parser --- */
    Parser parser;
    parser_init(&parser, &src);         /* primes two-token lookahead */

    /* --- Parse the whole program --- */
    Node *tree = parse_program(&parser); /* returns AST_PROGRAM node */

    /* --- Print the complete AST --- */
    printf("=== AST for '%s' ===\n\n", argv[1]);
    node_print(tree, 0);                /* depth=0 for the root */
    printf("\n");

    /* --- Summary line --- */
    printf("Parsed %d functions, had_error = %d\n",
           tree->nargs,         /* number of AST_FUNC children */
           parser.had_error);   /* 0 means clean parse */

    /* --- Clean up --- */
    node_free(tree);            /* release all AST memory */
    source_free(&src);          /* release source buffer */

    /* Return non-zero exit code if there were parse errors. */
    return parser.had_error ? 1 : 0;
}
