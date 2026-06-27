/*
 * main.c — Symbol Table demo driver
 *
 * Module: 03 — Symbol Table
 * Description: Lexes the source file given on argv[1], then does a simple
 *              token-pattern scan (NOT a full parse) to detect:
 *                - function definitions: IDENT followed by LPAREN
 *                - variable declarations: KW_INT followed by IDENT
 *              Each detected name is added to a SymTab and the table is
 *              dumped at the end.
 *
 * This is intentionally a rough heuristic demo — the goal is to exercise
 * the SymTab API, not to correctly parse all C constructs.
 *
 * Usage:  ./symtab_demo <source-file>
 */

#include "source.h"
#include "lexer.h"
#include "token.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MAX_TOKENS — upper bound on the token buffer used for lookahead.
 * We lex the entire file into an array first so we can look ahead
 * by one token without re-scanning. */
#define MAX_TOKENS 4096

/* token_text — copy a token's lexeme into a null-terminated buffer.
 * buf must be at least len+1 bytes.  Used for symtab_add which needs
 * a null-terminated string. */
static void token_text(const Token *tok, char *buf, size_t buflen)
{
    size_t n = tok->len < buflen - 1 ? tok->len : buflen - 1;
    memcpy(buf, tok->start, n);
    buf[n] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    printf("=== Symbol Table Demo — Module 03 ===\n\n");

    /* --- phase 1: lex the entire file into a flat token array --- */
    Source src = source_open(argv[1]);

    Lexer lex;
    lexer_init(&lex, &src);

    /* collect all tokens (including EOF) into an array for lookahead */
    Token tokens[MAX_TOKENS];
    int ntok = 0;
    do {
        if (ntok >= MAX_TOKENS) {
            fprintf(stderr, "error: too many tokens (max %d)\n", MAX_TOKENS);
            return 1;
        }
        tokens[ntok] = lexer_next(&lex);
    } while (tokens[ntok++].type != TOK_EOF);

    /* --- phase 2: token-pattern scan to populate the symbol table --- */
    SymTab st;
    symtab_init(&st);

    /* Heuristic parameter-count tracker.
     * When we detect IDENT LPAREN we start counting comma-separated
     * parameters until the matching RPAREN. */
    char name_buf[64]; /* scratch buffer for null-terminated lexeme copies */

    /*
     * Simple offset assignment strategy for this demo:
     *   - Variables get sequential negative offsets: -8, -16, -24, …
     *   - Functions get the parameter count (counted from the token stream).
     * A real compiler would compute offsets from type sizes.
     */
    int next_var_offset = -8; /* start below the saved frame pointer */

    for (int i = 0; i < ntok - 1; i++) {  /* ntok-1 because we look ahead by 1 */
        Token *cur  = &tokens[i];
        Token *next = &tokens[i + 1];

        /* --- detect function definition/declaration: IDENT ( --- */
        if (cur->type == TOK_IDENT && next->type == TOK_LPAREN) {
            token_text(cur, name_buf, sizeof(name_buf));

            /* count parameters by scanning forward to the matching ')' */
            int depth   = 0;  /* paren nesting depth */
            int params  = 0;  /* number of parameters detected */
            int in_params = 0; /* flag: are we inside the parameter list? */

            for (int j = i + 1; j < ntok; j++) {
                if (tokens[j].type == TOK_LPAREN) {
                    depth++;
                    in_params = 1; /* we've entered the parameter list */
                } else if (tokens[j].type == TOK_RPAREN) {
                    depth--;
                    if (depth == 0) break; /* found closing paren */
                } else if (tokens[j].type == TOK_COMMA && depth == 1) {
                    /* comma at depth 1 separates adjacent parameters */
                    params++;
                } else if (tokens[j].type == TOK_IDENT && depth == 1 && in_params) {
                    /* each IDENT at param-list depth that follows a KW_INT
                     * is a parameter name; we count the KW_INT instead */
                } else if (tokens[j].type == TOK_KW_INT && depth == 1) {
                    /* each 'int' keyword inside the param list is one parameter */
                    params++;
                } else if (tokens[j].type == TOK_KW_VOID && depth == 1) {
                    /* void parameter list means zero parameters */
                    params = 0;
                }
            }

            /* symtab_add returns -1 on duplicate — skip silently for this demo */
            int idx = symtab_add(&st, name_buf, SYM_FUNC, params);
            if (idx >= 0) {
                printf("Added: %s (FUNC, %d params)\n", name_buf, params);
            }
        }

        /* --- detect variable declaration: int IDENT --- */
        if (cur->type == TOK_KW_INT && next->type == TOK_IDENT) {
            /* look one more ahead to skip function parameter patterns
             * (we've already handled IDENT LPAREN above) */
            int look = i + 2;
            if (look < ntok && tokens[look].type == TOK_LPAREN) {
                /* this is actually a function declaration: int name(
                 * the IDENT LPAREN pattern above will handle it */
                continue;
            }

            token_text(next, name_buf, sizeof(name_buf));

            int idx = symtab_add(&st, name_buf, SYM_VAR, next_var_offset);
            if (idx >= 0) {
                printf("Added: %s (VAR, offset %d)\n", name_buf, next_var_offset);
                next_var_offset -= 8; /* each int occupies 8 bytes (aligned) */
            }
        }
    }

    /* --- phase 3: dump the completed symbol table --- */
    printf("\n");
    symtab_dump(&st);

    source_free(&src);
    return 0;
}
