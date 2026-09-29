/* lexer.c — Lexer implementation for bob_compiler
 * Module 02: Lexer
 */
#include "lexer.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static char peek_ch(Lexer *lx) {
    if (lx->src->pos >= lx->src->len) return '\0';
    return lx->src->text[lx->src->pos];
}

static char advance_ch(Lexer *lx) {
    char c = lx->src->text[lx->src->pos++];
    if (c == '\n') { lx->src->line++; lx->src->col = 1; }
    else           { lx->src->col++; }
    return c;
}

static void skip_whitespace(Lexer *lx) {
    for (;;) {
        while (isspace((unsigned char)peek_ch(lx))) advance_ch(lx);
        /* line comment */
        if (peek_ch(lx) == '/' &&
            lx->src->pos + 1 < lx->src->len &&
            lx->src->text[lx->src->pos + 1] == '/') {
            while (peek_ch(lx) != '\n' && peek_ch(lx) != '\0') advance_ch(lx);
            continue;
        }
        /* block comment */
        if (peek_ch(lx) == '/' &&
            lx->src->pos + 1 < lx->src->len &&
            lx->src->text[lx->src->pos + 1] == '*') {
            advance_ch(lx); advance_ch(lx);
            while (!(peek_ch(lx) == '*' &&
                     lx->src->pos + 1 < lx->src->len &&
                     lx->src->text[lx->src->pos + 1] == '/') &&
                   peek_ch(lx) != '\0') {
                advance_ch(lx);
            }
            if (peek_ch(lx) != '\0') { advance_ch(lx); advance_ch(lx); }
            continue;
        }
        break;
    }
}

static TokenType keyword_or_ident(const char *text) {
    if (strcmp(text, "int")    == 0) return TOK_KW_INT;
    if (strcmp(text, "void")   == 0) return TOK_KW_VOID;
    if (strcmp(text, "return") == 0) return TOK_KW_RETURN;
    if (strcmp(text, "if")     == 0) return TOK_KW_IF;
    if (strcmp(text, "else")   == 0) return TOK_KW_ELSE;
    if (strcmp(text, "while")  == 0) return TOK_KW_WHILE;
    return TOK_IDENT;
}

static Token lex_one(Lexer *lx) {
    skip_whitespace(lx);

    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.line = lx->src->line;
    tok.col  = lx->src->col;

    char c = peek_ch(lx);
    if (c == '\0') { tok.type = TOK_EOF; return tok; }

    if (isdigit((unsigned char)c)) {
        long val = 0;
        while (isdigit((unsigned char)peek_ch(lx)))
            val = val * 10 + (advance_ch(lx) - '0');
        tok.type = TOK_INT_LIT;
        tok.ival = val;
        return tok;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        int i = 0;
        while ((isalnum((unsigned char)peek_ch(lx)) || peek_ch(lx) == '_') && i < 63)
            tok.text[i++] = advance_ch(lx);
        tok.text[i] = '\0';
        tok.type = keyword_or_ident(tok.text);
        return tok;
    }

    advance_ch(lx);
    switch (c) {
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
        case '<':
            if (peek_ch(lx) == '=') { advance_ch(lx); tok.type = TOK_LEQ; }
            else tok.type = TOK_LT;
            break;
        case '>':
            if (peek_ch(lx) == '=') { advance_ch(lx); tok.type = TOK_GEQ; }
            else tok.type = TOK_GT;
            break;
        case '=':
            if (peek_ch(lx) == '=') { advance_ch(lx); tok.type = TOK_EQ; }
            else tok.type = TOK_ASSIGN;
            break;
        case '!':
            if (peek_ch(lx) == '=') { advance_ch(lx); tok.type = TOK_NEQ; }
            else tok.type = TOK_BANG;
            break;
        case '&':
            if (peek_ch(lx) == '&') { advance_ch(lx); tok.type = TOK_AMPAMP; }
            else tok.type = TOK_ERROR;
            break;
        case '|':
            if (peek_ch(lx) == '|') { advance_ch(lx); tok.type = TOK_PIPEPIPE; }
            else tok.type = TOK_ERROR;
            break;
        default:
            tok.type = TOK_ERROR;
            break;
    }
    return tok;
}

void lexer_init(Lexer *lx, Source *src) {
    lx->src     = src;
    lx->current = lex_one(lx);
}

Token lexer_next(Lexer *lx) {
    Token t     = lx->current;
    lx->current = lex_one(lx);
    return t;
}
