/*
 * lexer.h — Lexer interface for mycc
 * Module 16: The Complete Compiler
 */
#ifndef LEXER_H
#define LEXER_H

#include "source.h"
#include "token.h"

typedef struct {
    const char *src;      /* source text */
    size_t      src_len;
    size_t      pos;      /* current byte offset */
    int         line;
    int         col;
    const char *filename;
} Lexer;

/* Initialise lexer from a Source. */
void lexer_init(Lexer *lex, const Source *src);

/* Return the next token (advances position). */
Token lexer_next(Lexer *lex);

/* Peek at the next token without consuming it. */
Token lexer_peek(Lexer *lex);

#endif /* LEXER_H */
