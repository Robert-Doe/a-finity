/* lexer.c — Lexer implementation for my_compiler */
#include "lexer.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static char peek(Lexer *lx) {
    if (lx->src->pos >= lx->src->len) return '\0';
    return lx->src->text[lx->src->pos];
}

static char advance(Lexer *lx) {
    char c = lx->src->text[lx->src->pos++];
    if (c == '\n') { lx->src->line++; lx->src->col = 1; }
    else           { lx->src->col++; }
    return c;
}

static void skip_whitespace(Lexer *lx) {
    for (;;) {
        while (isspace((unsigned char)peek(lx))) advance(lx);
        if (peek(lx) == '/' && lx->src->pos + 1 < lx->src->len &&
            lx->src->text[lx->src->pos + 1] == '/') {
            while (peek(lx) != '\n' && peek(lx) != '\0') advance(lx);
            continue;
        }
        if (peek(lx) == '/' && lx->src->pos + 1 < lx->src->len &&
            lx->src->text[lx->src->pos + 1] == '*') {
            advance(lx); advance(lx);
            while (!(peek(lx) == '*' && lx->src->pos + 1 < lx->src->len &&
                     lx->src->text[lx->src->pos + 1] == '/') && peek(lx) != '\0')
                advance(lx);
            if (peek(lx) != '\0') { advance(lx); advance(lx); }
            continue;
        }
        break;
    }
}

static TokenType keyword_or_ident(const char *text) {
    if (strcmp(text, "int")    == 0) return TOK_INT;
    if (strcmp(text, "return") == 0) return TOK_RETURN;
    if (strcmp(text, "if")     == 0) return TOK_IF;
    if (strcmp(text, "else")   == 0) return TOK_ELSE;
    if (strcmp(text, "while")  == 0) return TOK_WHILE;
    if (strcmp(text, "void")   == 0) return TOK_VOID;
    return TOK_IDENT;
}

static Token lex_one(Lexer *lx) {
    skip_whitespace(lx);
    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.line = lx->src->line;
    tok.col  = lx->src->col;

    char c = peek(lx);
    if (c == '\0') { tok.type = TOK_EOF; return tok; }

    if (isdigit((unsigned char)c)) {
        long val = 0;
        while (isdigit((unsigned char)peek(lx)))
            val = val * 10 + (advance(lx) - '0');
        tok.type = TOK_INT_LIT;
        tok.ival = val;
        return tok;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        int i = 0;
        while ((isalnum((unsigned char)peek(lx)) || peek(lx) == '_') && i < 63)
            tok.text[i++] = advance(lx);
        tok.text[i] = '\0';
        tok.type = keyword_or_ident(tok.text);
        return tok;
    }

    advance(lx);
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
            if (peek(lx) == '=') { advance(lx); tok.type = TOK_LEQ; }
            else tok.type = TOK_LT;
            break;
        case '>':
            if (peek(lx) == '=') { advance(lx); tok.type = TOK_GEQ; }
            else tok.type = TOK_GT;
            break;
        case '=':
            if (peek(lx) == '=') { advance(lx); tok.type = TOK_EQEQ; }
            else tok.type = TOK_EQ;
            break;
        case '!':
            if (peek(lx) == '=') { advance(lx); tok.type = TOK_NEQ; }
            else tok.type = TOK_ERROR;
            break;
        case '&':
            if (peek(lx) == '&') { advance(lx); tok.type = TOK_AND; }
            else tok.type = TOK_ERROR;
            break;
        case '|':
            if (peek(lx) == '|') { advance(lx); tok.type = TOK_OR; }
            else tok.type = TOK_ERROR;
            break;
        default: tok.type = TOK_ERROR; break;
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
