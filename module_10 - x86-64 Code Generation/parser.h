/* parser.h — Recursive-descent parser for my_compiler
 * Module 06: Parser
 * Prerequisites: lexer.h, ast.h
 */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"
#include "ast.h"

/* Parser state. */
typedef struct {
    Lexer *lx;        /* token stream */
    int    had_error; /* set to 1 if any parse error occurred */
} Parser;

/* Initialise parser to read from lx. */
void  parser_init(Parser *p, Lexer *lx);

/* Parse a complete program and return its AST_PROGRAM node.
 * Returns NULL on fatal error. */
Node *parse_program(Parser *p);

#endif /* MY_COMPILER_PARSER_H */
