/*
 * lexer.h — Lexer struct and scanning API
 *
 * Module: 03 — Symbol Table (copied from Module 02)
 * Description: Declares the Lexer type and the two public functions
 *              (lexer_init, lexer_next) that drive tokenisation.
 *
 * Prerequisites: source.h (Source struct), token.h (Token, TokenType).
 */

#ifndef MY_COMPILER_LEXER_H
#define MY_COMPILER_LEXER_H

#include "source.h"  /* Source */
#include "token.h"   /* Token, TokenType */

/*
 * Lexer — mutable scanning cursor over a Source buffer.
 */
typedef struct {
    const Source *src;  /* source buffer being scanned — not owned by the Lexer */
    size_t        pos;  /* next character position within src->text */
    int           line; /* 1-based line of the character at pos */
    int           col;  /* 1-based column of the character at pos */
} Lexer;

/*
 * lexer_init — initialise a Lexer to start scanning from the beginning.
 */
void lexer_init(Lexer *lex, const Source *src);

/*
 * lexer_next — consume and return the next token from the source.
 */
Token lexer_next(Lexer *lex);

#endif /* MY_COMPILER_LEXER_H */
