/*
 * lexer.h — Lexer (tokenizer) interface
 * Module 11: Code Generation — Control Flow
 */

#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char *src;   /* pointer to beginning of source text */
    const char *cur;   /* current position */
    int         line;
    int         col;
} Lexer;

/* Initialize the lexer with a null-terminated source string. */
void lexer_init(Lexer *lex, const char *src);

/* Return the next token (advances the position). */
Token lexer_next(Lexer *lex);

/* Peek at the next token without advancing. */
Token lexer_peek(Lexer *lex);

#endif /* LEXER_H */
