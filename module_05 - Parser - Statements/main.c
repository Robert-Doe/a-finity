/* main.c -- Module 05: parse a C source file and print its AST.
 *
 * Usage: ./parser <source.c>
 *
 * This program:
 *   1. Opens and reads the source file into memory (source_open).
 *   2. Initialises a Lexer over that source.
 *   3. Initialises a Parser over the Lexer.
 *   4. Parses the entire program (parse_program).
 *   5. Prints the resulting AST (node_print).
 *   6. Frees all memory and exits cleanly.
 *
 * The only thing main() does is wire those pieces together.
 * Each piece is self-contained and tested independently.
 */
#include <stdio.h>   /* fprintf */
#include <stdlib.h>  /* exit */

#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <source.c>\n", argv[0]);
        return 1;
    }

    /* Step 1: load the file into memory.
     * source_open exits on I/O error, so we never see a NULL text pointer. */
    Source src = source_open(argv[1]);

    /* Step 2: create a lexer that reads from the source buffer. */
    Lexer lex;
    lexer_init(&lex, &src);

    /* Step 3: create the parser with two-token lookahead. */
    Parser p;
    parser_init(&p, &lex);

    /* Step 4: parse the whole program.
     * parse_program runs until TOK_EOF, collecting AST_FUNC nodes. */
    Node *tree = parse_program(&p);

    /* Step 5: print the AST starting at depth 0. */
    printf("=== AST for %s ===\n", argv[1]);
    node_print(tree, 0);

    /* Step 6: free everything we allocated. */
    node_free(tree);
    source_free(&src);

    return 0;
}
