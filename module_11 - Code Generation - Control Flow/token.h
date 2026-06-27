/*
 * token.h — Token types and Token struct
 * Module 11: Code Generation — Control Flow
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    /* Literals & identifiers */
    TOK_INT_LIT,   /* 42 */
    TOK_IDENT,     /* foo */

    /* Keywords */
    TOK_KW_INT,    /* int */
    TOK_KW_VOID,   /* void */
    TOK_KW_RETURN, /* return */
    TOK_KW_IF,     /* if */
    TOK_KW_ELSE,   /* else */
    TOK_KW_WHILE,  /* while */

    /* Arithmetic operators */
    TOK_PLUS,      /* + */
    TOK_MINUS,     /* - */
    TOK_STAR,      /* * */
    TOK_SLASH,     /* / */
    TOK_PERCENT,   /* % */

    /* Comparison operators */
    TOK_EQ,        /* == */
    TOK_NEQ,       /* != */
    TOK_LT,        /* < */
    TOK_GT,        /* > */
    TOK_LEQ,       /* <= */
    TOK_GEQ,       /* >= */

    /* Assignment */
    TOK_ASSIGN,    /* = */

    /* Logical operators */
    TOK_BANG,      /* ! */
    TOK_AMPAMP,    /* && */
    TOK_PIPEPIPE,  /* || */

    /* Delimiters */
    TOK_LPAREN,    /* ( */
    TOK_RPAREN,    /* ) */
    TOK_LBRACE,    /* { */
    TOK_RBRACE,    /* } */
    TOK_SEMICOLON, /* ; */
    TOK_COMMA,     /* , */

    /* Special */
    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType   type;
    const char *start; /* pointer into source buffer */
    size_t      len;   /* length of token text */
    int         line;
    int         col;
    long        ival;  /* filled for TOK_INT_LIT */
} Token;

/* Return a short human-readable name for a TokenType (for error messages). */
const char *token_type_name(TokenType t);

#endif /* TOKEN_H */
