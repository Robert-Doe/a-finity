/*
 * lexer.h — Module 03 interface (unchanged)
 *
 * The Lexer converts raw source text into a stream of Tokens one at a
 * time.  Call lexer_next() repeatedly; it returns TOK_EOF once the
 * entire input has been consumed.
 */

#ifndef LEXER_H
#define LEXER_H

#include "source.h"
#include "token.h"

/* ------------------------------------------------------------------ */
/* Lexer state                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *src_start; /* pointer to Source.text[0]              */
    const char *cur;       /* pointer to current scan position       */
    const char *end;       /* pointer one past the last byte         */
    int         line;      /* current 1-based line number            */
    int         col;       /* current 1-based column                 */
} Lexer;

/* Initialise a Lexer from an already-opened Source.
 * The Lexer does NOT own the Source; keep src alive until done.      */
void  lexer_init(Lexer *lx, const Source *src);

/* Scan and return the next token from the input.
 * Returns TOK_EOF once all input is exhausted.                       */
Token lexer_next(Lexer *lx);

#endif /* LEXER_H */
