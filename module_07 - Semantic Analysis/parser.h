/*
 * parser.h — Module 06 interface (unchanged)
 *
 * The Parser consumes a token stream produced by the Lexer and builds
 * an Abstract Syntax Tree (AST) represented with Node structs from ast.h.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/* ------------------------------------------------------------------ */
/* Parser state                                                        */
/* ------------------------------------------------------------------ */
typedef struct {
    Lexer  *lexer;      /* the lexer supplying tokens                 */
    Token   current;    /* the token we are currently looking at      */
    Token   previous;   /* the token we most recently consumed        */
    int     had_error;  /* set to 1 if any parse error occurred       */
} Parser;

/* Initialise the parser and prime it with the first token.          */
void  parser_init(Parser *p, Lexer *lx);

/* Parse the entire token stream and return the root AST node
 * (AST_PROGRAM).  Returns NULL on catastrophic failure.
 * Check p->had_error for non-fatal parse errors.                    */
Node *parse_program(Parser *p);

#endif /* PARSER_H */
