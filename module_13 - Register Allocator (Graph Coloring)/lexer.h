/* lexer.h — Lexer interface for bob_compiler
 * Module 02: Lexer
 * Prerequisites: source.h, token.h
 */
#ifndef BOB_LEXER_H
#define BOB_LEXER_H

#include "source.h"
#include "token.h"

typedef struct {
    Source *src;
    Token   current;
} Lexer;

void  lexer_init(Lexer *lx, Source *src);
Token lexer_next(Lexer *lx);

#endif /* BOB_LEXER_H */
