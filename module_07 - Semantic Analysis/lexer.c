/*
 * lexer.c — Module 03 implementation (unchanged)
 *
 * Hand-written lexer: skips whitespace/comments, then dispatches on
 * the first character to produce the next Token.
 */

#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/* Return the current character without advancing.                    */
static char peek(const Lexer *lx)
{
    if (lx->cur >= lx->end) return '\0';
    return *lx->cur;
}

/* Return the character after the current one (look-ahead by 1).     */
static char peek2(const Lexer *lx)
{
    if (lx->cur + 1 >= lx->end) return '\0';
    return *(lx->cur + 1);
}

/* Advance one character and update line/col counters.               */
static char advance(Lexer *lx)
{
    if (lx->cur >= lx->end) return '\0';
    char c = *lx->cur++;
    if (c == '\n') { lx->line++; lx->col = 1; }
    else           { lx->col++; }
    return c;
}

/* Skip whitespace and // line comments.                             */
static void skip_whitespace(Lexer *lx)
{
    for (;;) {
        /* skip blank characters */
        while (lx->cur < lx->end && isspace((unsigned char)peek(lx)))
            advance(lx);

        /* skip // comment to end of line */
        if (peek(lx) == '/' && peek2(lx) == '/') {
            while (lx->cur < lx->end && peek(lx) != '\n')
                advance(lx);
            continue;   /* re-enter loop to skip any following whitespace */
        }
        break;
    }
}

/* Build a simple (non-literal) token starting at `start`.          */
static Token make_tok(TokenType type, const char *start, int line, int col)
{
    Token t;
    t.type  = type;
    t.start = start;
    t.len   = 0;     /* filled in by caller if needed               */
    t.line  = line;
    t.col   = col;
    t.ival  = 0;
    return t;
}

/* ------------------------------------------------------------------ */
/* Keyword table                                                       */
/* ------------------------------------------------------------------ */
typedef struct { const char *word; TokenType type; } Keyword;

static const Keyword keywords[] = {
    { "int",    TOK_KW_INT    },
    { "void",   TOK_KW_VOID   },
    { "return", TOK_KW_RETURN },
    { "if",     TOK_KW_IF     },
    { "else",   TOK_KW_ELSE   },
    { "while",  TOK_KW_WHILE  },
    { NULL,     TOK_ERROR     }   /* sentinel */
};

/* Match a scanned identifier against the keyword table.             */
static TokenType keyword_or_ident(const char *start, size_t len)
{
    for (int i = 0; keywords[i].word; i++) {
        if (strlen(keywords[i].word) == len &&
            memcmp(keywords[i].word, start, len) == 0)
            return keywords[i].type;
    }
    return TOK_IDENT;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void lexer_init(Lexer *lx, const Source *src)
{
    lx->src_start = src->text;
    lx->cur       = src->text;
    lx->end       = src->text + src->len;
    lx->line      = 1;
    lx->col       = 1;
}

Token lexer_next(Lexer *lx)
{
    skip_whitespace(lx);

    if (lx->cur >= lx->end) {
        Token t = make_tok(TOK_EOF, lx->cur, lx->line, lx->col);
        t.len = 0;
        return t;
    }

    const char *start = lx->cur;
    int         line  = lx->line;
    int         col   = lx->col;
    char        c     = advance(lx);

    /* -- integer literal ------------------------------------------ */
    if (isdigit((unsigned char)c)) {
        while (isdigit((unsigned char)peek(lx))) advance(lx);
        Token t  = make_tok(TOK_INT_LIT, start, line, col);
        t.len    = (size_t)(lx->cur - start);
        t.ival   = strtol(start, NULL, 10);
        return t;
    }

    /* -- identifier / keyword ------------------------------------- */
    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)peek(lx)) || peek(lx) == '_')
            advance(lx);
        size_t    len  = (size_t)(lx->cur - start);
        TokenType type = keyword_or_ident(start, len);
        Token t = make_tok(type, start, line, col);
        t.len   = len;
        return t;
    }

    /* -- two-character operators ---------------------------------- */
    char nx = peek(lx);

#define TWO(a, b, kind)  if (c == (a) && nx == (b)) { \
        advance(lx);                                    \
        Token t = make_tok(kind, start, line, col);     \
        t.len = 2; return t; }

    TWO('=','=', TOK_EQ)
    TWO('!','=', TOK_NEQ)
    TWO('<','=', TOK_LEQ)
    TWO('>','=', TOK_GEQ)
    TWO('&','&', TOK_AMPAMP)
    TWO('|','|', TOK_PIPEPIPE)
#undef TWO

    /* -- single-character tokens ---------------------------------- */
    TokenType type = TOK_ERROR;
    switch (c) {
        case '+': type = TOK_PLUS;      break;
        case '-': type = TOK_MINUS;     break;
        case '*': type = TOK_STAR;      break;
        case '/': type = TOK_SLASH;     break;
        case '%': type = TOK_PERCENT;   break;
        case '<': type = TOK_LT;        break;
        case '>': type = TOK_GT;        break;
        case '=': type = TOK_ASSIGN;    break;
        case '!': type = TOK_BANG;      break;
        case '(': type = TOK_LPAREN;    break;
        case ')': type = TOK_RPAREN;    break;
        case '{': type = TOK_LBRACE;    break;
        case '}': type = TOK_RBRACE;    break;
        case ';': type = TOK_SEMICOLON; break;
        case ',': type = TOK_COMMA;     break;
        default:  type = TOK_ERROR;     break;
    }
    Token t = make_tok(type, start, line, col);
    t.len   = 1;
    return t;
}

/* ------------------------------------------------------------------ */
/* token_type_name — lives here so token.h stays header-only          */
/* ------------------------------------------------------------------ */
const char *token_type_name(TokenType t)
{
    switch (t) {
        case TOK_INT_LIT:   return "TOK_INT_LIT";
        case TOK_IDENT:     return "TOK_IDENT";
        case TOK_KW_INT:    return "TOK_KW_INT";
        case TOK_KW_VOID:   return "TOK_KW_VOID";
        case TOK_KW_RETURN: return "TOK_KW_RETURN";
        case TOK_KW_IF:     return "TOK_KW_IF";
        case TOK_KW_ELSE:   return "TOK_KW_ELSE";
        case TOK_KW_WHILE:  return "TOK_KW_WHILE";
        case TOK_PLUS:      return "TOK_PLUS";
        case TOK_MINUS:     return "TOK_MINUS";
        case TOK_STAR:      return "TOK_STAR";
        case TOK_SLASH:     return "TOK_SLASH";
        case TOK_PERCENT:   return "TOK_PERCENT";
        case TOK_EQ:        return "TOK_EQ";
        case TOK_NEQ:       return "TOK_NEQ";
        case TOK_LT:        return "TOK_LT";
        case TOK_GT:        return "TOK_GT";
        case TOK_LEQ:       return "TOK_LEQ";
        case TOK_GEQ:       return "TOK_GEQ";
        case TOK_ASSIGN:    return "TOK_ASSIGN";
        case TOK_BANG:      return "TOK_BANG";
        case TOK_AMPAMP:    return "TOK_AMPAMP";
        case TOK_PIPEPIPE:  return "TOK_PIPEPIPE";
        case TOK_LPAREN:    return "TOK_LPAREN";
        case TOK_RPAREN:    return "TOK_RPAREN";
        case TOK_LBRACE:    return "TOK_LBRACE";
        case TOK_RBRACE:    return "TOK_RBRACE";
        case TOK_SEMICOLON: return "TOK_SEMICOLON";
        case TOK_COMMA:     return "TOK_COMMA";
        case TOK_EOF:       return "TOK_EOF";
        case TOK_ERROR:     return "TOK_ERROR";
        default:            return "TOK_UNKNOWN";
    }
}
