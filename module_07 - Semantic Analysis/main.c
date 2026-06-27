/*
 * main.c — Module 07 driver
 *
 * Usage:  sema <source-file>
 *
 * Pipeline:
 *   1. Open the source file         (source_open)
 *   2. Lex + Parse into an AST      (lexer_init / parse_program)
 *   3. Run semantic analysis        (sema_check)
 *   4. Report results and exit
 *
 * Exit codes:
 *   0  — semantic check passed (no errors)
 *   1  — semantic errors found OR usage/IO error
 */

#include <stdio.h>
#include <stdlib.h>

#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "sema.h"

int main(int argc, char *argv[])
{
    /* ---- argument check ---------------------------------------- */
    if (argc != 2) {
        fprintf(stderr, "usage: sema <source-file>\n");
        return 1;
    }

    /* ---- open source file -------------------------------------- */
    Source *src = source_open(argv[1]);
    if (!src) return 1;   /* source_open already printed an error   */

    /* ---- lex --------------------------------------------------- */
    Lexer lx;
    lexer_init(&lx, src);

    /* ---- parse ------------------------------------------------- */
    Parser parser;
    parser_init(&parser, &lx);
    Node *program = parse_program(&parser);

    if (parser.had_error) {
        fprintf(stderr, "parse errors found — aborting semantic check\n");
        node_free(program);
        source_free(src);
        return 1;
    }

    /* ---- semantic analysis ------------------------------------- */
    int nerrors = sema_check(program);

    /* ---- report ------------------------------------------------ */
    if (nerrors == 0) {
        printf("semantic check passed\n");
    } else {
        printf("%d semantic error%s\n", nerrors, nerrors == 1 ? "" : "s");
    }

    /* ---- clean up ---------------------------------------------- */
    node_free(program);
    source_free(src);

    return nerrors > 0 ? 1 : 0;
}
