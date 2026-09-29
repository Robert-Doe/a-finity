/*
 * lexer.h — Lexer struct and scanning API
 *
 * Module: 02 — Lexer / Tokeniser
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
 *
 * Fields:
 *   src  — read-only pointer to the Source being tokenised.
 *   pos  — byte offset of the next character to be consumed (0-based).
 *   line — current 1-based line number (updated whenever '\n' is consumed).
 *   col  — current 1-based column number (reset to 1 after each newline).
 */
typedef struct {
    const Source *src;  /* source buffer being scanned — not owned by the Lexer */
    size_t        pos;  /* next character position within src->text */
    int           line; /* 1-based line of the character at pos */
    int           col;  /* 1-based column of the character at pos */
} Lexer;

/*
 * lexer_init — initialise a Lexer to start scanning from the beginning.
 *
 * lex: pointer to an uninitialised Lexer to set up.
 * src: pointer to a fully loaded Source (must outlive the Lexer).
 */
void lexer_init(Lexer *lex, const Source *src);

/*
 * lexer_next — consume and return the next token from the source.
 *
 * lex: pointer to a Lexer previously initialised by lexer_init.
 * Returns: the next Token. When the end of file is reached every
 *          subsequent call returns a token of type TOK_EOF.
 *          Unknown characters produce TOK_ERROR (one character wide).
 */
Token lexer_next(Lexer *lex);

#endif /* MY_COMPILER_LEXER_H */
