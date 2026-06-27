/*
 * main.c — Lexer demo driver
 *
 * Module: 02 — Lexer / Tokeniser
 * Description: Opens a C source file given on the command line, runs the
 *              lexer over it, and prints every token with its location,
 *              type name, and lexeme text.
 *
 * Usage:  ./lexer_demo <source-file>
 * Example output:
 *   [  1:  1] KW_int     "int"
 *   [  1:  5] IDENT      "add"
 *   ...
 *   42 tokens produced.
 */

#include "source.h"
#include "lexer.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    /* require exactly one command-line argument: the source file path */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    /* load the entire source file into a heap buffer */
    Source src = source_open(argv[1]);

    /* set up the lexer cursor at position 0, line 1, col 1 */
    Lexer lex;
    lexer_init(&lex, &src);

    int token_count = 0; /* running total for the summary line */

    /* scan until the EOF sentinel — lexer_next is safe to call after EOF */
    Token tok;
    do {
        tok = lexer_next(&lex);
        token_count++;

        /* print: [line:col] TYPE_NAME "lexeme"
         * %-10s left-aligns the type name in a 10-char field for readability */
        printf("[%3d:%3d] %-10s \"%.*s\"\n",
               tok.line,
               tok.col,
               token_type_name(tok.type),
               (int)tok.len,   /* printf width for a non-null-terminated string */
               tok.start);

    } while (tok.type != TOK_EOF); /* stop after printing the EOF token itself */

    /* summary line so the user can see total token count at a glance */
    printf("\n%d tokens produced.\n", token_count);

    /* clean up the source buffer — good habit even though the process exits */
    source_free(&src);
    return 0;
}
