/* lexer.h — Lexer (tokeniser) for my_compiler
 * Module 02 — introduced here, unchanged across all modules.
 * Prerequisites: Module 01 (source.h), token.h.
 */
#ifndef MY_COMPILER_LEXER_H
#define MY_COMPILER_LEXER_H

#include "source.h"
#include "token.h"

/* Lexer: iterates over a Source, yielding one Token at a time.
 * All fields are maintained internally by lexer_next; callers should not
 * modify them directly. */
typedef struct {
    const Source *src;  /* source being scanned (not owned) */
    size_t        pos;  /* current byte offset into src->text */
    int           line; /* current 1-based line number */
    int           col;  /* current 1-based column number */
} Lexer;

/* lexer_init: initialise *lex to scan *src from the beginning. */
void  lexer_init(Lexer *lex, const Source *src);

/* lexer_next: scan and return the next token.
 * Returns TOK_EOF once the end of input is reached (and on every subsequent
 * call).  Returns TOK_ERROR for any unrecognised character. */
Token lexer_next(Lexer *lex);

#endif /* MY_COMPILER_LEXER_H */
