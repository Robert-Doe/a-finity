/* parser.h — Recursive descent parser for my_compiler
 * Module 04 — Parser struct and API introduced here.
 * Prerequisites: Module 02 (lexer.h), Module 04 (ast.h).
 *
 * In Module 04 only parse_expr is implemented.
 * parse_block and parse_stmt are added in Module 05.
 * parse_program and parse_function are added in Module 06.
 */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"  /* Lexer, Token */
#include "ast.h"    /* Node */

/* Parser: wraps a Lexer and provides one token of "current" lookahead.
 *
 * The parser always has the next unconsumed token in p->current.
 * When it consumes a token (via advance()) the consumed token moves to
 * p->prev so error messages can refer to "the token we just consumed".
 */
typedef struct {
    Lexer  lex;        /* the underlying lexer (owns the scan position) */
    Token  current;    /* the token we are looking at right now */
    Token  prev;       /* the token we most recently consumed */
    int    had_error;  /* set to 1 if any parse error occurred */
} Parser;

/* parser_init: initialise *p to parse the given Source.
 * Primes p->current with the first real token. */
void  parser_init(Parser *p, const Source *src);

/* parse_program: parse a complete program (list of functions).
 * Implemented in Module 06.  Returns NULL in Modules 04 and 05. */
Node *parse_program(Parser *p);

/* parse_function: parse one function definition.
 * Implemented in Module 06.  Returns NULL in Modules 04 and 05. */
Node *parse_function(Parser *p);

/* parse_block: parse a braced block: '{' stmt* '}'
 * Implemented in Module 05.  Returns NULL in Module 04. */
Node *parse_block(Parser *p);

/* parse_stmt: parse a single statement.
 * Implemented in Module 05.  Returns NULL in Module 04. */
Node *parse_stmt(Parser *p);

/* parse_expr: parse a full expression (entry point for expression parsing).
 * Implemented in Module 04. */
Node *parse_expr(Parser *p);

#endif /* MY_COMPILER_PARSER_H */
