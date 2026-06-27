/* parser.h — Parser interface for my_compiler
 * Module 04-06: Parser
 * Prerequisites: lexer.h, ast.h
 */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"
#include "ast.h"

/* Parser state. */
typedef struct {
    Lexer  *lx;         /* the lexer providing tokens */
    int     had_error;  /* set to 1 if any parse error occurred */
} Parser;

/* Parse the entire source file and return the AST_PROGRAM root.
 * Returns NULL on catastrophic failure. */
Node *parse_program(Parser *p, Lexer *lx);

#endif /* MY_COMPILER_PARSER_H */
