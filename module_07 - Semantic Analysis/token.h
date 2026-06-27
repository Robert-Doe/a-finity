/*
 * token.h — Module 02 interface (unchanged)
 *
 * Defines every token kind the lexer can produce and the Token struct
 * that carries position + value information for each scanned token.
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>   /* size_t */

/* ------------------------------------------------------------------ */
/* Token kinds                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    /* literals & identifiers */
    TOK_INT_LIT,    /* 42, 0, -1 …                                    */
    TOK_IDENT,      /* foo, main, x …                                 */

    /* keywords */
    TOK_KW_INT,     /* int                                            */
    TOK_KW_VOID,    /* void                                           */
    TOK_KW_RETURN,  /* return                                         */
    TOK_KW_IF,      /* if                                             */
    TOK_KW_ELSE,    /* else                                           */
    TOK_KW_WHILE,   /* while                                          */

    /* arithmetic operators */
    TOK_PLUS,       /* +                                              */
    TOK_MINUS,      /* -                                              */
    TOK_STAR,       /* *                                              */
    TOK_SLASH,      /* /                                              */
    TOK_PERCENT,    /* %                                              */

    /* comparison operators */
    TOK_EQ,         /* ==                                             */
    TOK_NEQ,        /* !=                                             */
    TOK_LT,         /* <                                              */
    TOK_GT,         /* >                                              */
    TOK_LEQ,        /* <=                                             */
    TOK_GEQ,        /* >=                                             */

    /* assignment */
    TOK_ASSIGN,     /* =                                              */

    /* logical / bitwise */
    TOK_BANG,       /* !                                              */
    TOK_AMPAMP,     /* &&                                             */
    TOK_PIPEPIPE,   /* ||                                             */

    /* delimiters */
    TOK_LPAREN,     /* (                                              */
    TOK_RPAREN,     /* )                                              */
    TOK_LBRACE,     /* {                                              */
    TOK_RBRACE,     /* }                                              */
    TOK_SEMICOLON,  /* ;                                              */
    TOK_COMMA,      /* ,                                              */

    /* sentinels */
    TOK_EOF,        /* end of input                                   */
    TOK_ERROR       /* unrecognised character                        */
} TokenType;

/* ------------------------------------------------------------------ */
/* Token                                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    TokenType   type;   /* what kind of token this is                 */
    const char *start;  /* pointer into Source.text (NOT owned)       */
    size_t      len;    /* number of bytes in the lexeme              */
    int         line;   /* 1-based line number                        */
    int         col;    /* 1-based column of the first character      */
    long        ival;   /* numeric value for TOK_INT_LIT              */
} Token;

/* Return a human-readable name for a token kind, e.g. "TOK_PLUS".
 * The returned string is a string literal — do not free it.          */
const char *token_type_name(TokenType t);

#endif /* TOKEN_H */
