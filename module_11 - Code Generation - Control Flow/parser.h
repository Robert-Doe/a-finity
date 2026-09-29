/*
 * parser.h — Recursive-descent parser interface
 * Module 11: Code Generation — Control Flow
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer   lex;
    Token   cur;    /* current (already consumed) look-ahead token */
    int     errors;
} Parser;

/* Initialize parser with source text. */
void   parser_init(Parser *p, const char *src);

/* Parse a full translation unit; returns AST_PROGRAM node. */
Node  *parser_parse(Parser *p);

#endif /* PARSER_H */
