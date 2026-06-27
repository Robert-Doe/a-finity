/*
 * lexer.c — Full lexer / tokeniser implementation
 *
 * File:        lexer.c
 * Module:      03 — Symbol Table (copied from Module 02)
 * Description: Converts a Source buffer into a stream of Tokens one at a
 *              time.  The public entry points are lexer_init() and
 *              lexer_next().  Everything else is file-local.
 * Prerequisites: source.h (Source), token.h (Token/TokenType), lexer.h (Lexer)
 *
 * Compile:  gcc -Wall -Wextra -Werror -std=c11 -c lexer.c
 */

#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * Internal character-class helpers
 * ---------------------------------------------------------------------- */

/*
 * file_is_alpha_or_underscore — test whether ch can start an identifier.
 */
static int file_is_alpha_or_underscore(char ch)
{
    return (isalpha((unsigned char)ch) || ch == '_');
}

/*
 * file_is_alnum_or_underscore — test whether ch can continue an identifier.
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
 */
static char advance(Lexer *lex)
{
    char ch = lex->src->text[lex->pos];
    lex->pos++;

    if (ch == '\n') {
        lex->line++;
        lex->col = 1;
    } else {
        lex->col++;
    }

    return ch;
}

/*
 * peek — look at the character at lex->pos without consuming it.
 */
static char peek(const Lexer *lex)
{
    return lex->src->text[lex->pos];
}

/*
 * peek_next — look one character ahead of peek() without consuming.
 */
static char peek_next(const Lexer *lex)
{
    if (lex->pos + 1 >= lex->src->len) return '\0';
    return lex->src->text[lex->pos + 1];
}

/* -------------------------------------------------------------------------
 * Whitespace and comment skipping
 * ---------------------------------------------------------------------- */

/*
 * skip_whitespace_and_comments — advance past spaces, tabs, newlines, and
 * both styles of C comment.
 */
static void skip_whitespace_and_comments(Lexer *lex)
{
    for (;;) {
        while (isspace((unsigned char)peek(lex))) {
            advance(lex);
        }

        if (peek(lex) == '/' && peek_next(lex) == '/') {
            advance(lex);
            advance(lex);
            while (peek(lex) != '\n' && peek(lex) != '\0') {
                advance(lex);
            }
            continue;
        }

        if (peek(lex) == '/' && peek_next(lex) == '*') {
            advance(lex);
            advance(lex);
            while (!(peek(lex) == '*' && peek_next(lex) == '/') &&
                   peek(lex) != '\0') {
                advance(lex);
            }
            if (peek(lex) == '*') {
                advance(lex);
                advance(lex);
            }
            continue;
        }

        break;
    }
}

/* -------------------------------------------------------------------------
 * Keyword table
 * ---------------------------------------------------------------------- */

typedef struct {
    const char *word;
    TokenType   type;
} KwEntry;

static const KwEntry kw_table[] = {
    { "int",    TOK_KW_INT    },
    { "void",   TOK_KW_VOID   },
    { "return", TOK_KW_RETURN },
    { "if",     TOK_KW_IF     },
    { "else",   TOK_KW_ELSE   },
    { "while",  TOK_KW_WHILE  },
};

static const size_t KW_COUNT = sizeof(kw_table) / sizeof(kw_table[0]);

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void lexer_init(Lexer *lex, const Source *src)
{
    lex->src  = src;
    lex->pos  = 0;
    lex->line = 1;
    lex->col  = 1;
}

Token lexer_next(Lexer *lex)
{
    skip_whitespace_and_comments(lex);

    Token tok;
    tok.ival  = 0;

    tok.start = lex->src->text + lex->pos;
    tok.line  = lex->line;
    tok.col   = lex->col;

    if (peek(lex) == '\0') {
        tok.type = TOK_EOF;
        tok.len  = 0;
        return tok;
    }

    char ch = peek(lex);

    if (isdigit((unsigned char)ch)) {
        size_t start_pos = lex->pos;
        while (isdigit((unsigned char)peek(lex))) {
            advance(lex);
        }
        tok.type = TOK_INT_LIT;
        tok.len  = lex->pos - start_pos;
        tok.ival = strtol(tok.start, NULL, 10);
        return tok;
    }

    if (file_is_alpha_or_underscore(ch)) {
        size_t start_pos = lex->pos;
        while (file_is_alnum_or_underscore(peek(lex))) {
            advance(lex);
        }
        tok.len  = lex->pos - start_pos;
        tok.type = TOK_IDENT;

        for (size_t i = 0; i < KW_COUNT; i++) {
            size_t kw_len = strlen(kw_table[i].word);
            if (kw_len == tok.len &&
                strncmp(tok.start, kw_table[i].word, tok.len) == 0) {
                tok.type = kw_table[i].type;
                break;
            }
        }
        return tok;
    }

    advance(lex);
    tok.len = 1;

    switch (ch) {
        case '=':
            if (peek(lex) == '=') { advance(lex); tok.type = TOK_EQ;       tok.len = 2; }
            else                  {               tok.type = TOK_ASSIGN;              }
            break;
        case '!':
            if (peek(lex) == '=') { advance(lex); tok.type = TOK_NEQ;       tok.len = 2; }
            else                  {               tok.type = TOK_BANG;                }
            break;
        case '<':
            if (peek(lex) == '=') { advance(lex); tok.type = TOK_LEQ;       tok.len = 2; }
            else                  {               tok.type = TOK_LT;                  }
            break;
        case '>':
            if (peek(lex) == '=') { advance(lex); tok.type = TOK_GEQ;       tok.len = 2; }
            else                  {               tok.type = TOK_GT;                  }
            break;
        case '&':
            if (peek(lex) == '&') { advance(lex); tok.type = TOK_AMPAMP;    tok.len = 2; }
            else                  {               tok.type = TOK_ERROR;               }
            break;
        case '|':
            if (peek(lex) == '|') { advance(lex); tok.type = TOK_PIPEPIPE;  tok.len = 2; }
            else                  {               tok.type = TOK_ERROR;               }
            break;

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
            tok.type = TOK_ERROR;
            fprintf(stderr, "lexer error at %d:%d: unexpected character '%c'\n",
                    tok.line, tok.col, ch);
            break;
    }

    return tok;
}

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
