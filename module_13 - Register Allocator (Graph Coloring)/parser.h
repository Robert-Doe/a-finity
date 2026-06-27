/* parser.h — Recursive-descent parser for bob_compiler
 * Module 06: Parser
 */
#ifndef BOB_PARSER_H
#define BOB_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lx;
    int    had_error;
} Parser;

void  parser_init(Parser *p, Lexer *lx);
Node *parse_program(Parser *p);

#endif /* BOB_PARSER_H */
