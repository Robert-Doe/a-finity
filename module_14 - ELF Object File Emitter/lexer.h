/* lexer.h — Lexer interface for my_compiler */
#ifndef MY_COMPILER_LEXER_H
#define MY_COMPILER_LEXER_H

#include "source.h"
#include "token.h"

typedef struct {
    Source *src;
    Token   current;
} Lexer;

void  lexer_init(Lexer *lx, Source *src);
Token lexer_next(Lexer *lx);

#endif /* MY_COMPILER_LEXER_H */
