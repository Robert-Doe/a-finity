/*
 * lexer.c — Lexer implementation
 * Module 11: Code Generation — Control Flow
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lexer.h"

void lexer_init(Lexer *lex, const char *src) {
    lex->src  = src;
    lex->cur  = src;
    lex->line = 1;
    lex->col  = 1;
}

/* ------------------------------------------------------------------ helpers */

static void skip_whitespace_and_comments(Lexer *lex) {
    for (;;) {
        /* Skip whitespace */
        while (*lex->cur && isspace((unsigned char)*lex->cur)) {
            if (*lex->cur == '\n') { lex->line++; lex->col = 1; }
            else { lex->col++; }
            lex->cur++;
        }
        /* Skip // line comments */
        if (lex->cur[0] == '/' && lex->cur[1] == '/') {
            while (*lex->cur && *lex->cur != '\n') lex->cur++;
            continue;
        }
        /* Skip /* block comments */
        if (lex->cur[0] == '/' && lex->cur[1] == '*') {
            lex->cur += 2; lex->col += 2;
            while (*lex->cur && !(lex->cur[0] == '*' && lex->cur[1] == '/')) {
                if (*lex->cur == '\n') { lex->line++; lex->col = 1; }
                else { lex->col++; }
                lex->cur++;
            }
            if (*lex->cur) { lex->cur += 2; lex->col += 2; }
            continue;
        }
        break;
    }
}

static Token make_tok(TokenType type, const char *start, size_t len,
                      int line, int col) {
    Token t;
    t.type  = type;
    t.start = start;
    t.len   = len;
    t.line  = line;
    t.col   = col;
    t.ival  = 0;
    return t;
}

/* ----------------------------------------------------------------- keywords */

static TokenType keyword_or_ident(const char *s, size_t len) {
    switch (len) {
    case 2: if (memcmp(s,"if",2)==0) return TOK_KW_IF; break;
    case 3: if (memcmp(s,"int",3)==0) return TOK_KW_INT; break;
    case 4: if (memcmp(s,"void",4)==0) return TOK_KW_VOID;
            if (memcmp(s,"else",4)==0) return TOK_KW_ELSE; break;
    case 5: if (memcmp(s,"while",5)==0) return TOK_KW_WHILE; break;
    case 6: if (memcmp(s,"return",6)==0) return TOK_KW_RETURN; break;
    default: break;
    }
    return TOK_IDENT;
}

/* --------------------------------------------------------------- next token */

static Token lex_one(Lexer *lex) {
    skip_whitespace_and_comments(lex);

    const char *start = lex->cur;
    int line = lex->line, col = lex->col;

    if (!*lex->cur)
        return make_tok(TOK_EOF, start, 0, line, col);

    char c = *lex->cur++;
    lex->col++;

    /* Integer literal */
    if (isdigit((unsigned char)c)) {
        while (isdigit((unsigned char)*lex->cur)) { lex->cur++; lex->col++; }
        Token t = make_tok(TOK_INT_LIT, start, (size_t)(lex->cur - start), line, col);
        t.ival = strtol(start, NULL, 10);
        return t;
    }

    /* Identifier / keyword */
    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)*lex->cur) || *lex->cur == '_') {
            lex->cur++; lex->col++;
        }
        size_t len = (size_t)(lex->cur - start);
        return make_tok(keyword_or_ident(start, len), start, len, line, col);
    }

    /* Two-character operators */
#define PEEK (lex->cur[0])
    switch (c) {
    case '=': if (PEEK=='=') { lex->cur++; lex->col++; return make_tok(TOK_EQ, start,2,line,col); }
              return make_tok(TOK_ASSIGN,start,1,line,col);
    case '!': if (PEEK=='=') { lex->cur++; lex->col++; return make_tok(TOK_NEQ,start,2,line,col); }
              return make_tok(TOK_BANG,start,1,line,col);
    case '<': if (PEEK=='=') { lex->cur++; lex->col++; return make_tok(TOK_LEQ,start,2,line,col); }
              return make_tok(TOK_LT,start,1,line,col);
    case '>': if (PEEK=='=') { lex->cur++; lex->col++; return make_tok(TOK_GEQ,start,2,line,col); }
              return make_tok(TOK_GT,start,1,line,col);
    case '&': if (PEEK=='&') { lex->cur++; lex->col++; return make_tok(TOK_AMPAMP,start,2,line,col); }
              break;
    case '|': if (PEEK=='|') { lex->cur++; lex->col++; return make_tok(TOK_PIPEPIPE,start,2,line,col); }
              break;
    /* Single-character tokens */
    case '+': return make_tok(TOK_PLUS,     start,1,line,col);
    case '-': return make_tok(TOK_MINUS,    start,1,line,col);
    case '*': return make_tok(TOK_STAR,     start,1,line,col);
    case '/': return make_tok(TOK_SLASH,    start,1,line,col);
    case '%': return make_tok(TOK_PERCENT,  start,1,line,col);
    case '(': return make_tok(TOK_LPAREN,   start,1,line,col);
    case ')': return make_tok(TOK_RPAREN,   start,1,line,col);
    case '{': return make_tok(TOK_LBRACE,   start,1,line,col);
    case '}': return make_tok(TOK_RBRACE,   start,1,line,col);
    case ';': return make_tok(TOK_SEMICOLON,start,1,line,col);
    case ',': return make_tok(TOK_COMMA,    start,1,line,col);
    default:  break;
    }
#undef PEEK

    fprintf(stderr, "lexer error: unexpected char '%c' at line %d\n", c, line);
    return make_tok(TOK_ERROR, start, 1, line, col);
}

Token lexer_next(Lexer *lex) {
    return lex_one(lex);
}

Token lexer_peek(Lexer *lex) {
    Lexer save = *lex;
    Token t = lex_one(lex);
    *lex = save;
    return t;
}

/* token_type_name lives here for simplicity */
const char *token_type_name(TokenType t) {
    switch (t) {
    case TOK_INT_LIT:   return "INT_LIT";
    case TOK_IDENT:     return "IDENT";
    case TOK_KW_INT:    return "'int'";
    case TOK_KW_VOID:   return "'void'";
    case TOK_KW_RETURN: return "'return'";
    case TOK_KW_IF:     return "'if'";
    case TOK_KW_ELSE:   return "'else'";
    case TOK_KW_WHILE:  return "'while'";
    case TOK_PLUS:      return "'+'";
    case TOK_MINUS:     return "'-'";
    case TOK_STAR:      return "'*'";
    case TOK_SLASH:     return "'/'";
    case TOK_PERCENT:   return "'%'";
    case TOK_EQ:        return "'=='";
    case TOK_NEQ:       return "'!='";
    case TOK_LT:        return "'<'";
    case TOK_GT:        return "'>'";
    case TOK_LEQ:       return "'<='";
    case TOK_GEQ:       return "'>='";
    case TOK_ASSIGN:    return "'='";
    case TOK_BANG:      return "'!'";
    case TOK_AMPAMP:    return "'&&'";
    case TOK_PIPEPIPE:  return "'||'";
    case TOK_LPAREN:    return "'('";
    case TOK_RPAREN:    return "')'";
    case TOK_LBRACE:    return "'{'";
    case TOK_RBRACE:    return "'}'";
    case TOK_SEMICOLON: return "';'";
    case TOK_COMMA:     return "','";
    case TOK_EOF:       return "EOF";
    case TOK_ERROR:     return "ERROR";
    default:            return "?";
    }
}
