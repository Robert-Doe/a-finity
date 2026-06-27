/* parser.h — Recursive-descent parser for my_compiler
 * Module 05 — two-token lookahead added; Module 06 — parse_function and
 * parse_program fully implemented.
 * Prerequisites: source.h, lexer.h, ast.h.
 */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"
#include "ast.h"

/* Parser: state for the recursive-descent parser.
 *
 * We keep two tokens of lookahead (current + next) so that we can
 * distinguish a function definition from an expression statement at the
 * top level and inside blocks without backtracking.
 *
 *   current  — the token being examined right now
 *   next     — the token immediately after current (one step ahead)
 *   prev     — the most recently consumed token (useful for error messages)
 *   had_error — set to 1 on any parse error; parse continues to collect more
 */
typedef struct {
    Lexer  lex;        /* the underlying token stream */
    Token  current;    /* token currently under examination */
    Token  next;       /* one extra lookahead token */
    Token  prev;       /* most recently consumed token */
    int    had_error;  /* non-zero if any error has been reported */
} Parser;

/* parser_init: initialise the parser to scan *src.
 * Primes both current and next lookahead slots. */
void  parser_init(Parser *p, const Source *src);

/* parse_program: parse a sequence of function definitions.
 * Returns an AST_PROGRAM node whose args[] holds AST_FUNC nodes. */
Node *parse_program(Parser *p);

/* parse_function: parse one function definition.
 * Grammar: ('int'|'void') IDENT '(' param_list ')' block
 * Returns an AST_FUNC node. */
Node *parse_function(Parser *p);

/* parse_block: parse a brace-enclosed statement list.
 * Grammar: '{' stmt* '}'
 * Returns an AST_BLOCK node. */
Node *parse_block(Parser *p);

/* parse_stmt: parse a single statement.
 * Dispatches to var-decl, if, while, return, assignment, or expression. */
Node *parse_stmt(Parser *p);

/* parse_expr: parse a full expression (handles precedence internally).
 * Entry point for operator-precedence parsing. */
Node *parse_expr(Parser *p);

#endif /* MY_COMPILER_PARSER_H */
