/* parser.h — Parser interface for my_compiler
 * Module 15: Standard Library Shim
 * Prerequisites: source.h, token.h, lexer.h, ast.h
 */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"
#include "ast.h"

/* Parser state. */
typedef struct {
    Lexer *lx;          /* the lexer providing tokens */
    int    had_error;   /* 1 if any parse error occurred */
} Parser;

/* Parse a complete program from lx into an AST_PROGRAM node.
 * Returns the root node (caller must free with node_free). */
Node *parse_program(Parser *p, Lexer *lx);

#endif /* MY_COMPILER_PARSER_H */
