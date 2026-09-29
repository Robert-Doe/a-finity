/*
 * parser.h — Parser interface for mycc
 * Module 16: The Complete Compiler
 */
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer  *lex;
    Token   cur;      /* current (look-ahead) token */
    int     errors;
} Parser;

void     parser_init(Parser *p, Lexer *lex);
ASTNode *parse_program(Parser *p);

#endif /* PARSER_H */
