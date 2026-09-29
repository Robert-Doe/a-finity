/* parser.h — Recursive-descent parser for my_compiler */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lx;
    int    had_error;
} Parser;

void  parser_init(Parser *p, Lexer *lx);
Node *parse_program(Parser *p);

#endif /* MY_COMPILER_PARSER_H */
