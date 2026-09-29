/*
 * lexer.c — Full lexer / tokeniser implementation
 *
 * File:        lexer.c
 * Module:      02 — Lexer / Tokeniser
 * Description: Converts a Source buffer into a stream of Tokens one at a
 *              time.  The public entry points are lexer_init() and
 *              lexer_next().  Everything else is file-local.
 * Prerequisites: source.h (Source), token.h (Token/TokenType), lexer.h (Lexer)
 *
 * Compile:  gcc -Wall -Wextra -Werror -std=c11 -c lexer.c
 */

#include "lexer.h"

#include <ctype.h>   /* isdigit, isalpha, isspace */
#include <stdlib.h>  /* strtol */
#include <string.h>  /* strcmp, memset */
#include <stdio.h>   /* fprintf (for error messages) */

/* -------------------------------------------------------------------------
 * Internal character-class helpers
 * ---------------------------------------------------------------------- */

/*
 * file_is_alpha_or_underscore — test whether ch can start an identifier.
 *
 * ch: character to test.
 * Returns: 1 if ch is a letter or underscore, 0 otherwise.
 * Why: identifiers and keywords must begin with [A-Za-z_].
 */
static int file_is_alpha_or_underscore(char ch)
{
    return (isalpha((unsigned char)ch) || ch == '_');
}

/*
 * file_is_alnum_or_underscore — test whether ch can continue an identifier.
 *
 * ch: character to test.
 * Returns: 1 if ch is a letter, digit or underscore, 0 otherwise.
 * Why: after the first character an identifier may also contain digits.
 */
static int file_is_alnum_or_underscore(char ch)
{
    return (isalnum((unsigned char)ch) || ch == '_');
}

/* -------------------------------------------------------------------------
 * Cursor movement helpers
 * ---------------------------------------------------------------------- */

/*
 * advance — consume one character and return it, updating line/col.
 *
 * lex: the active Lexer.
 * Returns: the character that was at lex->pos before advancing.
 * Assumes: lex->pos < lex->src->len  (caller must guard against EOF).
 * Why: centralising the advance step ensures line/col bookkeeping is
 *      never missed.
 */
static char advance(Lexer *lex)
{
    char ch = lex->src->text[lex->pos]; /* read the current character */
    lex->pos++;                          /* move past it */

    if (ch == '\n') {
        /* newline: bump line counter and reset column to before first char */
        lex->line++;
        lex->col = 1;
    } else {
        /* ordinary character: just widen the column */
        lex->col++;
    }

    return ch;
}

/*
 * peek — look at the character at lex->pos without consuming it.
 *
 * lex: the active Lexer.
 * Returns: the next character, or '\0' when at end of file.
 * Why: '\0' is safe because source_open null-terminates the buffer, so
 *      the character after the last byte is always '\0'.
 */
static char peek(const Lexer *lex)
{
    return lex->src->text[lex->pos]; /* '\0' if pos == len */
}

/*
 * peek_next — look one character ahead of peek() without consuming.
 *
 * lex: the active Lexer.
 * Returns: the character at pos+1, or '\0' if past the buffer.
 * Why: needed to distinguish == from = and similar two-char operators.
 */
static char peek_next(const Lexer *lex)
{
    if (lex->pos + 1 >= lex->src->len) return '\0'; /* guard buffer end */
    return lex->src->text[lex->pos + 1];
}

/* -------------------------------------------------------------------------
 * Whitespace and comment skipping
 * ---------------------------------------------------------------------- */

/*
 * skip_whitespace_and_comments — advance past spaces, tabs, newlines, and
 * both styles of C comment.
 *
 * lex: the active Lexer; pos/line/col are updated in place.
 * Assumes: lex->src is valid.
 * Note: handles // line comments and slash-star block comments.
 *       Unterminated block comments are silently consumed to EOF.
 * Why: the parser never needs to see whitespace or comments, so we
 *      discard them in the lexer rather than adding special tokens.
 */
static void skip_whitespace_and_comments(Lexer *lex)
{
    for (;;) {
        /* skip all contiguous whitespace characters */
        while (isspace((unsigned char)peek(lex))) {
            advance(lex); /* consume the whitespace character */
        }

        /* check for // line comment */
        if (peek(lex) == '/' && peek_next(lex) == '/') {
            advance(lex); /* consume first '/' */
            advance(lex); /* consume second '/' */
            /* read until newline or EOF — the newline is NOT consumed here
             * so the next iteration's isspace() will handle it and bump line */
            while (peek(lex) != '\n' && peek(lex) != '\0') {
                advance(lex);
            }
            continue; /* restart the outer loop to skip the newline */
        }

        /* check for slash-star block comment */
        if (peek(lex) == '/' && peek_next(lex) == '*') {
            advance(lex); /* consume '/' */
            advance(lex); /* consume '*' */
            /* scan until closing star-slash or EOF */
            while (!(peek(lex) == '*' && peek_next(lex) == '/') &&
                   peek(lex) != '\0') {
                advance(lex); /* consume comment body characters */
            }
            if (peek(lex) == '*') {
                advance(lex); /* consume '*' of closing */
                advance(lex); /* consume '/' of closing */
            }
            continue; /* restart loop — there may be more whitespace after */
        }

        break; /* no more whitespace or comments to skip */
    }
}

/* -------------------------------------------------------------------------
 * Keyword table
 * ---------------------------------------------------------------------- */

/*
 * KwEntry — one row in the keyword lookup table.
 */
typedef struct {
    const char *word; /* the keyword string exactly as it appears in source */
    TokenType   type; /* the corresponding token type */
} KwEntry;

/*
 * kw_table — static array mapping keyword strings to their token types.
 *
 * Why a table: adding a new keyword requires only one new row rather
 * than editing a chain of if/else or switch cases.
 */
static const KwEntry kw_table[] = {
    { "int",    TOK_KW_INT    },
    { "void",   TOK_KW_VOID   },
    { "return", TOK_KW_RETURN },
    { "if",     TOK_KW_IF     },
    { "else",   TOK_KW_ELSE   },
    { "while",  TOK_KW_WHILE  },
};

/* number of entries in kw_table */
static const size_t KW_COUNT = sizeof(kw_table) / sizeof(kw_table[0]);

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/*
 * lexer_init — prepare a Lexer to scan from the beginning of src.
 *
 * lex: uninitialised Lexer to set up.
 * src: fully loaded Source whose text will be tokenised.
 * Note: does not allocate heap memory; all state lives in the Lexer struct.
 */
void lexer_init(Lexer *lex, const Source *src)
{
    lex->src  = src; /* point at the source buffer — not owned */
    lex->pos  = 0;   /* start at byte zero */
    lex->line = 1;   /* editors and compilers number lines from 1 */
    lex->col  = 1;   /* columns also start at 1 */
}

/*
 * lexer_next — scan and return the next token.
 *
 * lex: Lexer positioned at the start of the next lexeme.
 * Returns: the next Token. The token's start pointer points into
 *          lex->src->text and must not be freed.
 * Note: advances lex->pos past the returned lexeme. Callers keep
 *       calling lexer_next until tok.type == TOK_EOF.
 */
Token lexer_next(Lexer *lex)
{
    /* discard whitespace and comments before the next real token */
    skip_whitespace_and_comments(lex);

    Token tok;
    tok.ival  = 0;   /* default; only meaningful for TOK_INT_LIT */

    /* record position BEFORE consuming characters so the token carries
     * the location where it starts, not where it ends */
    tok.start = lex->src->text + lex->pos;
    tok.line  = lex->line;
    tok.col   = lex->col;

    /* --- end of file --- */
    if (peek(lex) == '\0') {
        tok.type = TOK_EOF;
        tok.len  = 0;
        return tok;
    }

    char ch = peek(lex); /* examine but do not yet consume */

    /* --- integer literals: one or more decimal digits --- */
    if (isdigit((unsigned char)ch)) {
        size_t start_pos = lex->pos; /* remember where the literal begins */
        while (isdigit((unsigned char)peek(lex))) {
            advance(lex); /* consume each digit */
        }
        tok.type = TOK_INT_LIT;
        tok.len  = lex->pos - start_pos; /* number of digit characters */
        /* parse numeric value from the raw text — strtol handles leading zeros */
        tok.ival = strtol(tok.start, NULL, 10);
        return tok;
    }

    /* --- identifiers and keywords --- */
    if (file_is_alpha_or_underscore(ch)) {
        size_t start_pos = lex->pos;
        while (file_is_alnum_or_underscore(peek(lex))) {
            advance(lex); /* consume identifier characters */
        }
        tok.len  = lex->pos - start_pos;
        tok.type = TOK_IDENT; /* assume identifier until proven to be a keyword */

        /* linear scan of keyword table: compare lexeme against each keyword */
        for (size_t i = 0; i < KW_COUNT; i++) {
            size_t kw_len = strlen(kw_table[i].word);
            if (kw_len == tok.len &&
                strncmp(tok.start, kw_table[i].word, tok.len) == 0) {
                tok.type = kw_table[i].type; /* upgrade from IDENT to keyword */
                break;
            }
        }
        return tok;
    }

    /* --- multi-character and single-character operators / punctuation ---
     * Consume the first character unconditionally, then peek at the next
     * to decide between one- and two-character forms. */
    advance(lex); /* consume ch */
    tok.len = 1;  /* most tokens are one character */

    switch (ch) {
        /* ---- two-char possibilities ---- */
        case '=':
            if (peek(lex) == '=') {  /* == comparison */
                advance(lex);
                tok.type = TOK_EQ;
                tok.len  = 2;
            } else {
                tok.type = TOK_ASSIGN; /* plain assignment */
            }
            break;

        case '!':
            if (peek(lex) == '=') {  /* != comparison */
                advance(lex);
                tok.type = TOK_NEQ;
                tok.len  = 2;
            } else {
                tok.type = TOK_BANG;  /* logical not */
            }
            break;

        case '<':
            if (peek(lex) == '=') {  /* <= comparison */
                advance(lex);
                tok.type = TOK_LEQ;
                tok.len  = 2;
            } else {
                tok.type = TOK_LT;   /* strict less-than */
            }
            break;

        case '>':
            if (peek(lex) == '=') {  /* >= comparison */
                advance(lex);
                tok.type = TOK_GEQ;
                tok.len  = 2;
            } else {
                tok.type = TOK_GT;   /* strict greater-than */
            }
            break;

        case '&':
            if (peek(lex) == '&') {  /* && logical and */
                advance(lex);
                tok.type = TOK_AMPAMP;
                tok.len  = 2;
            } else {
                /* single & is not in our language — report as error */
                tok.type = TOK_ERROR;
            }
            break;

        case '|':
            if (peek(lex) == '|') {  /* || logical or */
                advance(lex);
                tok.type = TOK_PIPEPIPE;
                tok.len  = 2;
            } else {
                /* single | is not in our language — report as error */
                tok.type = TOK_ERROR;
            }
            break;

        /* ---- unambiguous single-character tokens ---- */
        case '+': tok.type = TOK_PLUS;      break;
        case '-': tok.type = TOK_MINUS;     break;
        case '*': tok.type = TOK_STAR;      break;
        case '/': tok.type = TOK_SLASH;     break;
        case '%': tok.type = TOK_PERCENT;   break;
        case '(': tok.type = TOK_LPAREN;    break;
        case ')': tok.type = TOK_RPAREN;    break;
        case '{': tok.type = TOK_LBRACE;    break;
        case '}': tok.type = TOK_RBRACE;    break;
        case ';': tok.type = TOK_SEMICOLON; break;
        case ',': tok.type = TOK_COMMA;     break;

        default:
            /* unknown character: emit TOK_ERROR and let the parser decide
             * what to do — we do NOT call exit() here so recovery is possible */
            tok.type = TOK_ERROR;
            fprintf(stderr, "lexer error at %d:%d: unexpected character '%c'\n",
                    tok.line, tok.col, ch);
            break;
    }

    return tok;
}

/*
 * token_type_name — map a TokenType to a printable string.
 *
 * type: any value from the TokenType enum.
 * Returns: a static constant string (never NULL, never needs to be freed).
 * Note: the strings intentionally omit the "TOK_" prefix for brevity in
 *       diagnostic output.
 */
const char *token_type_name(TokenType type)
{
    switch (type) {
        case TOK_INT_LIT:   return "INT_LIT";
        case TOK_IDENT:     return "IDENT";
        case TOK_KW_INT:    return "KW_int";
        case TOK_KW_VOID:   return "KW_void";
        case TOK_KW_RETURN: return "KW_return";
        case TOK_KW_IF:     return "KW_if";
        case TOK_KW_ELSE:   return "KW_else";
        case TOK_KW_WHILE:  return "KW_while";
        case TOK_PLUS:      return "PLUS";
        case TOK_MINUS:     return "MINUS";
        case TOK_STAR:      return "STAR";
        case TOK_SLASH:     return "SLASH";
        case TOK_PERCENT:   return "PERCENT";
        case TOK_EQ:        return "EQ";
        case TOK_NEQ:       return "NEQ";
        case TOK_LT:        return "LT";
        case TOK_GT:        return "GT";
        case TOK_LEQ:       return "LEQ";
        case TOK_GEQ:       return "GEQ";
        case TOK_ASSIGN:    return "ASSIGN";
        case TOK_BANG:      return "BANG";
        case TOK_AMPAMP:    return "AMPAMP";
        case TOK_PIPEPIPE:  return "PIPEPIPE";
        case TOK_LPAREN:    return "LPAREN";
        case TOK_RPAREN:    return "RPAREN";
        case TOK_LBRACE:    return "LBRACE";
        case TOK_RBRACE:    return "RBRACE";
        case TOK_SEMICOLON: return "SEMICOLON";
        case TOK_COMMA:     return "COMMA";
        case TOK_EOF:       return "EOF";
        case TOK_ERROR:     return "ERROR";
        default:            return "UNKNOWN";
    }
}
