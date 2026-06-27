/* parser.h -- Recursive descent parser for my_compiler
 * Module 05 -- statement parsing introduced here.
 * Prerequisites: Module 02 (lexer.h), Module 04 (ast.h).
 *
 * In Module 05 we add full statement and program parsing.
 * The Parser uses two-token lookahead (cur + peek) to disambiguate
 * constructs like "int x;" (var decl) vs a function definition at
 * the top level.
 */
#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "lexer.h"  /* Lexer, Token */
#include "ast.h"    /* Node */

/* Parser: wraps a Lexer with two-token lookahead.
 *
 * WHY two tokens?
 *   Inside a function body, "int" can start either a variable declaration
 *   ("int x;") or -- in later modules -- a nested function. With two tokens
 *   we can see both "int" and the identifier that follows before committing
 *   to a parse path.  Having both cur and peek pre-fetched keeps the code
 *   simple: no manual "un-get" is needed.
 */
typedef struct {
    Lexer  lex;     /* the underlying lexer (owns the scan position) */
    Token  cur;     /* the token we are looking at right now */
    Token  peek;    /* the token after cur (one step of lookahead) */
} Parser;

/* parser_init: initialise *p to parse tokens produced by *lex.
 * Primes both p->cur and p->peek with the first two real tokens. */
void  parser_init(Parser *p, Lexer *lex);

/* parse_program: parse a complete program -- one or more function definitions.
 * Returns an AST_PROGRAM node whose args[] array holds AST_FUNC nodes. */
Node *parse_program(Parser *p);

/* parse_func: parse one function definition of the form:
 *   "int" IDENT "(" params ")" block
 * Returns an AST_FUNC node. */
Node *parse_func(Parser *p);

/* parse_block: parse a braced block of statements: "{" stmt* "}"
 * Returns an AST_BLOCK node whose args[] holds the statement nodes. */
Node *parse_block(Parser *p);

/* parse_stmt: parse a single statement.
 * Dispatches based on the current token to:
 *   TOK_LBRACE    -> parse_block
 *   TOK_KW_IF     -> if statement
 *   TOK_KW_WHILE  -> while loop
 *   TOK_KW_RETURN -> return statement
 *   TOK_KW_INT    -> variable declaration
 *   anything else -> expression statement
 */
Node *parse_stmt(Parser *p);

/* parse_expr: parse a full expression including assignment.
 * Precedence (lowest to highest):
 *   assignment (right-assoc), logical or/and, comparison,
 *   additive, multiplicative, unary, primary/call.
 */
Node *parse_expr(Parser *p);

#endif /* MY_COMPILER_PARSER_H */
