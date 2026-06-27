/* lexer.h — Lexer interface for my_compiler
 * Module 15: Standard Library Shim
 * Prerequisites: source.h, token.h
 */
#ifndef MY_COMPILER_LEXER_H
#define MY_COMPILER_LEXER_H

#include "source.h"
#include "token.h"

/* Lexer state: wraps a Source and remembers one token of lookahead. */
typedef struct {
    Source *src;        /* the source being tokenised */
    Token   current;    /* the most recently lexed token */
} Lexer;

/* Initialise lx to read from src, and lex the first token. */
void  lexer_init(Lexer *lx, Source *src);

/* Advance to the next token and return it. */
Token lexer_next(Lexer *lx);

#endif /* MY_COMPILER_LEXER_H */
