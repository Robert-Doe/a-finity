/*
 * lexer.c — Lexer implementation for mycc
 * Module 16: The Complete Compiler
 */
#include "lexer.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void lexer_init(Lexer *lex, const Source *src)
{
    lex->src      = src->text;
    lex->src_len  = src->length;
    lex->pos      = 0;
    lex->line     = 1;
    lex->col      = 1;
    lex->filename = src->filename;
}

static char peek_char(const Lexer *lex)
{
    if (lex->pos >= lex->src_len) return '\0';
    return lex->src[lex->pos];
}

static char next_char(Lexer *lex)
{
    char c = lex->src[lex->pos++];
    if (c == '\n') { lex->line++; lex->col = 1; }
    else           { lex->col++; }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lex)
{
    for (;;) {
        /* whitespace */
        while (lex->pos < lex->src_len && isspace((unsigned char)peek_char(lex)))
            next_char(lex);

        /* line comment // */
        if (lex->pos + 1 < lex->src_len &&
            lex->src[lex->pos] == '/' && lex->src[lex->pos+1] == '/') {
            while (lex->pos < lex->src_len && peek_char(lex) != '\n')
                next_char(lex);
            continue;
        }

        /* block comment /* */
        if (lex->pos + 1 < lex->src_len &&
            lex->src[lex->pos] == '/' && lex->src[lex->pos+1] == '*') {
            next_char(lex); next_char(lex); /* consume /  * */
            while (lex->pos + 1 < lex->src_len) {
                if (lex->src[lex->pos] == '*' && lex->src[lex->pos+1] == '/') {
                    next_char(lex); next_char(lex);
                    break;
                }
                next_char(lex);
            }
            continue;
        }
        break;
    }
}

static Token make_tok(TokenType type, const char *start, size_t len, int line, int col)
{
    Token t;
    t.type  = type;
    t.start = start;
    t.len   = len;
    t.line  = line;
    t.col   = col;
    t.ival  = 0;
    return t;
}

/* Keyword table */
static struct { const char *word; size_t wlen; TokenType type; } kw_table[] = {
    { "int",    3, TOK_KW_INT    },
    { "return", 6, TOK_KW_RETURN },
    { "if",     2, TOK_KW_IF     },
    { "else",   4, TOK_KW_ELSE   },
    { "while",  5, TOK_KW_WHILE  },
    { "void",   4, TOK_KW_VOID   },
    { "print",  5, TOK_KW_PRINT  },
};
#define KW_COUNT (int)(sizeof kw_table / sizeof kw_table[0])

Token lexer_next(Lexer *lex)
{
    skip_whitespace_and_comments(lex);

    if (lex->pos >= lex->src_len) {
        return make_tok(TOK_EOF, lex->src + lex->pos, 0, lex->line, lex->col);
    }

    int    line  = lex->line;
    int    col   = lex->col;
    size_t start = lex->pos;
    char   c     = next_char(lex);

    /* Integer literal */
    if (isdigit((unsigned char)c)) {
        while (lex->pos < lex->src_len && isdigit((unsigned char)peek_char(lex)))
            next_char(lex);
        Token t = make_tok(TOK_INT_LIT, lex->src + start, lex->pos - start, line, col);
        t.ival  = strtol(lex->src + start, NULL, 10);
        return t;
    }

    /* Identifier or keyword */
    if (isalpha((unsigned char)c) || c == '_') {
        while (lex->pos < lex->src_len &&
               (isalnum((unsigned char)peek_char(lex)) || peek_char(lex) == '_'))
            next_char(lex);
        size_t len = lex->pos - start;
        for (int i = 0; i < KW_COUNT; i++) {
            if (kw_table[i].wlen == len &&
                memcmp(lex->src + start, kw_table[i].word, len) == 0)
                return make_tok(kw_table[i].type, lex->src + start, len, line, col);
        }
        return make_tok(TOK_IDENT, lex->src + start, len, line, col);
    }

    /* Two-character operators */
    if (c == '=' && peek_char(lex) == '=') { next_char(lex); return make_tok(TOK_EQ,  lex->src+start, 2, line, col); }
    if (c == '!' && peek_char(lex) == '=') { next_char(lex); return make_tok(TOK_NEQ, lex->src+start, 2, line, col); }
    if (c == '<' && peek_char(lex) == '=') { next_char(lex); return make_tok(TOK_LE,  lex->src+start, 2, line, col); }
    if (c == '>' && peek_char(lex) == '=') { next_char(lex); return make_tok(TOK_GE,  lex->src+start, 2, line, col); }
    if (c == '&' && peek_char(lex) == '&') { next_char(lex); return make_tok(TOK_AND, lex->src+start, 2, line, col); }
    if (c == '|' && peek_char(lex) == '|') { next_char(lex); return make_tok(TOK_OR,  lex->src+start, 2, line, col); }

    /* Single-character operators and punctuation */
    switch (c) {
        case '(': return make_tok(TOK_LPAREN,    lex->src+start, 1, line, col);
        case ')': return make_tok(TOK_RPAREN,    lex->src+start, 1, line, col);
        case '{': return make_tok(TOK_LBRACE,    lex->src+start, 1, line, col);
        case '}': return make_tok(TOK_RBRACE,    lex->src+start, 1, line, col);
        case ';': return make_tok(TOK_SEMICOLON, lex->src+start, 1, line, col);
        case ',': return make_tok(TOK_COMMA,     lex->src+start, 1, line, col);
        case '+': return make_tok(TOK_PLUS,      lex->src+start, 1, line, col);
        case '-': return make_tok(TOK_MINUS,     lex->src+start, 1, line, col);
        case '*': return make_tok(TOK_STAR,      lex->src+start, 1, line, col);
        case '/': return make_tok(TOK_SLASH,     lex->src+start, 1, line, col);
        case '%': return make_tok(TOK_PERCENT,   lex->src+start, 1, line, col);
        case '<': return make_tok(TOK_LT,        lex->src+start, 1, line, col);
        case '>': return make_tok(TOK_GT,        lex->src+start, 1, line, col);
        case '=': return make_tok(TOK_ASSIGN,    lex->src+start, 1, line, col);
        case '!': return make_tok(TOK_NOT,       lex->src+start, 1, line, col);
        default:
            fprintf(stderr, "%s:%d:%d: unexpected character '%c'\n",
                    lex->filename, line, col, c);
            return make_tok(TOK_EOF, lex->src+start, 1, line, col);
    }
}

Token lexer_peek(Lexer *lex)
{
    Lexer saved = *lex;
    Token t = lexer_next(lex);
    *lex = saved;
    return t;
}

const char *token_type_name(TokenType t)
{
    switch (t) {
        case TOK_INT_LIT:   return "INT_LIT";
        case TOK_IDENT:     return "IDENT";
        case TOK_KW_INT:    return "int";
        case TOK_KW_RETURN: return "return";
        case TOK_KW_IF:     return "if";
        case TOK_KW_ELSE:   return "else";
        case TOK_KW_WHILE:  return "while";
        case TOK_KW_VOID:   return "void";
        case TOK_KW_PRINT:  return "print";
        case TOK_LPAREN:    return "(";
        case TOK_RPAREN:    return ")";
        case TOK_LBRACE:    return "{";
        case TOK_RBRACE:    return "}";
        case TOK_SEMICOLON: return ";";
        case TOK_COMMA:     return ",";
        case TOK_PLUS:      return "+";
        case TOK_MINUS:     return "-";
        case TOK_STAR:      return "*";
        case TOK_SLASH:     return "/";
        case TOK_PERCENT:   return "%";
        case TOK_EQ:        return "==";
        case TOK_NEQ:       return "!=";
        case TOK_LT:        return "<";
        case TOK_LE:        return "<=";
        case TOK_GT:        return ">";
        case TOK_GE:        return ">=";
        case TOK_ASSIGN:    return "=";
        case TOK_AND:       return "&&";
        case TOK_OR:        return "||";
        case TOK_NOT:       return "!";
        case TOK_EOF:       return "EOF";
        default:            return "?";
    }
}
