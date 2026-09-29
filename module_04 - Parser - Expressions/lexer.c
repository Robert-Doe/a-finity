/* lexer.c — Lexer implementation for my_compiler
 * Module 02 — introduced here, unchanged across all modules.
 * Prerequisites: Module 01 (source.h), token.h, lexer.h.
 */
#include "lexer.h"

#include <ctype.h>   /* isdigit, isalpha, isalnum, isspace */
#include <string.h>  /* strncmp */
#include <stdlib.h>  /* strtol */
#include <stdio.h>   /* fprintf */

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/* current character, or '\0' at end of input */
static char peek(const Lexer *lex) {
    if (lex->pos >= lex->src->len) return '\0';
    return lex->src->text[lex->pos];
}

/* one character ahead of current, or '\0' */
static char peek2(const Lexer *lex) {
    if (lex->pos + 1 >= lex->src->len) return '\0';
    return lex->src->text[lex->pos + 1];
}

/* advance one byte, updating line/col tracking */
static void advance_char(Lexer *lex) {
    if (lex->pos >= lex->src->len) return;
    if (lex->src->text[lex->pos] == '\n') {
        lex->line++;
        lex->col = 1;
    } else {
        lex->col++;
    }
    lex->pos++;
}

/* skip whitespace and // line comments */
static void skip_whitespace(Lexer *lex) {
    for (;;) {
        while (isspace((unsigned char)peek(lex))) advance_char(lex);
        /* // comment: skip to end of line */
        if (peek(lex) == '/' && peek2(lex) == '/') {
            while (peek(lex) != '\0' && peek(lex) != '\n') advance_char(lex);
        } else {
            break;
        }
    }
}

/* make a simple single-character token */
static Token make_tok(const Lexer *lex, TokenType type,
                      const char *start, int line, int col) {
    Token t;
    t.type  = type;
    t.start = start;
    t.len   = (size_t)(lex->src->text + lex->pos - start);
    t.line  = line;
    t.col   = col;
    t.ival  = 0;
    return t;
}

/* keyword table */
static const struct { const char *word; size_t len; TokenType type; } KEYWORDS[] = {
    { "int",    3, TOK_KW_INT    },
    { "void",   4, TOK_KW_VOID   },
    { "return", 6, TOK_KW_RETURN },
    { "if",     2, TOK_KW_IF     },
    { "else",   4, TOK_KW_ELSE   },
    { "while",  5, TOK_KW_WHILE  },
};
#define NKEYWORDS (sizeof(KEYWORDS)/sizeof(KEYWORDS[0]))

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void lexer_init(Lexer *lex, const Source *src) {
    lex->src  = src;
    lex->pos  = 0;
    lex->line = 1;
    lex->col  = 1;
}

Token lexer_next(Lexer *lex) {
    skip_whitespace(lex);

    int         line  = lex->line;
    int         col   = lex->col;
    const char *start = lex->src->text + lex->pos;
    Token       t;

    if (lex->pos >= lex->src->len) {
        /* End of input */
        t.type = TOK_EOF; t.start = start; t.len = 0;
        t.line = line; t.col = col; t.ival = 0;
        return t;
    }

    char c = peek(lex);
    advance_char(lex);

    switch (c) {
    case '+': return make_tok(lex, TOK_PLUS,      start, line, col);
    case '-': return make_tok(lex, TOK_MINUS,     start, line, col);
    case '*': return make_tok(lex, TOK_STAR,      start, line, col);
    case '/': return make_tok(lex, TOK_SLASH,     start, line, col);
    case '%': return make_tok(lex, TOK_PERCENT,   start, line, col);
    case '(': return make_tok(lex, TOK_LPAREN,    start, line, col);
    case ')': return make_tok(lex, TOK_RPAREN,    start, line, col);
    case '{': return make_tok(lex, TOK_LBRACE,    start, line, col);
    case '}': return make_tok(lex, TOK_RBRACE,    start, line, col);
    case ';': return make_tok(lex, TOK_SEMICOLON, start, line, col);
    case ',': return make_tok(lex, TOK_COMMA,     start, line, col);

    case '=':
        if (peek(lex) == '=') { advance_char(lex); return make_tok(lex, TOK_EQ,     start, line, col); }
        return make_tok(lex, TOK_ASSIGN, start, line, col);
    case '!':
        if (peek(lex) == '=') { advance_char(lex); return make_tok(lex, TOK_NEQ,    start, line, col); }
        return make_tok(lex, TOK_BANG,   start, line, col);
    case '<':
        if (peek(lex) == '=') { advance_char(lex); return make_tok(lex, TOK_LEQ,    start, line, col); }
        return make_tok(lex, TOK_LT,     start, line, col);
    case '>':
        if (peek(lex) == '=') { advance_char(lex); return make_tok(lex, TOK_GEQ,    start, line, col); }
        return make_tok(lex, TOK_GT,     start, line, col);
    case '&':
        if (peek(lex) == '&') { advance_char(lex); return make_tok(lex, TOK_AMPAMP,   start, line, col); }
        break;
    case '|':
        if (peek(lex) == '|') { advance_char(lex); return make_tok(lex, TOK_PIPEPIPE, start, line, col); }
        break;

    default:
        break;
    }

    /* Integer literal */
    if (isdigit((unsigned char)c)) {
        while (isdigit((unsigned char)peek(lex))) advance_char(lex);
        t = make_tok(lex, TOK_INT_LIT, start, line, col);
        t.ival = strtol(start, NULL, 10);
        return t;
    }

    /* Identifier or keyword */
    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)peek(lex)) || peek(lex) == '_') advance_char(lex);
        t = make_tok(lex, TOK_IDENT, start, line, col);
        /* Check against keyword table */
        for (size_t i = 0; i < NKEYWORDS; i++) {
            if (t.len == KEYWORDS[i].len &&
                strncmp(t.start, KEYWORDS[i].word, t.len) == 0) {
                t.type = KEYWORDS[i].type;
                break;
            }
        }
        return t;
    }

    /* Unrecognised character */
    t = make_tok(lex, TOK_ERROR, start, line, col);
    fprintf(stderr, "lexer error at line %d col %d: unexpected character '%c'\n",
            line, col, c);
    return t;
}

/* token_type_name: return printable name for a token type */
const char *token_type_name(TokenType type) {
    switch (type) {
    case TOK_INT_LIT:   return "INT_LIT";
    case TOK_IDENT:     return "IDENT";
    case TOK_KW_INT:    return "KW_INT";
    case TOK_KW_VOID:   return "KW_VOID";
    case TOK_KW_RETURN: return "KW_RETURN";
    case TOK_KW_IF:     return "KW_IF";
    case TOK_KW_ELSE:   return "KW_ELSE";
    case TOK_KW_WHILE:  return "KW_WHILE";
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
    }
    return "UNKNOWN";
}
