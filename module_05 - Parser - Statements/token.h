/* token.h — Token types and Token struct for my_compiler
 * Module 02 — introduced here, unchanged across all modules.
 * Prerequisites: Module 01 (source.h).
 */
#ifndef MY_COMPILER_TOKEN_H
#define MY_COMPILER_TOKEN_H

#include <stddef.h>  /* size_t */

/* TokenType: every distinct kind of token the lexer can produce. */
typedef enum {
    TOK_INT_LIT,    /* integer literal, e.g. 42 */
    TOK_IDENT,      /* identifier, e.g. foo */

    /* Keywords */
    TOK_KW_INT,     /* int */
    TOK_KW_VOID,    /* void */
    TOK_KW_RETURN,  /* return */
    TOK_KW_IF,      /* if */
    TOK_KW_ELSE,    /* else */
    TOK_KW_WHILE,   /* while */

    /* Arithmetic operators */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_STAR,       /* * */
    TOK_SLASH,      /* / */
    TOK_PERCENT,    /* % */

    /* Comparison operators */
    TOK_EQ,         /* == */
    TOK_NEQ,        /* != */
    TOK_LT,         /* < */
    TOK_GT,         /* > */
    TOK_LEQ,        /* <= */
    TOK_GEQ,        /* >= */

    /* Logical / assignment / unary */
    TOK_ASSIGN,     /* = */
    TOK_BANG,       /* ! */
    TOK_AMPAMP,     /* && */
    TOK_PIPEPIPE,   /* || */

    /* Punctuation */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_LBRACE,     /* { */
    TOK_RBRACE,     /* } */
    TOK_SEMICOLON,  /* ; */
    TOK_COMMA,      /* , */

    /* Sentinels */
    TOK_EOF,        /* end of input */
    TOK_ERROR       /* unrecognised character */
} TokenType;

/* Token: a single lexical token produced by the lexer.
 * The token does NOT own its text — start points into the Source buffer. */
typedef struct {
    TokenType   type;   /* what kind of token this is */
    const char *start;  /* pointer into Source.text where this token begins */
    size_t      len;    /* number of characters in the token */
    int         line;   /* 1-based source line number */
    int         col;    /* 1-based source column number */
    long        ival;   /* parsed integer value (only valid for TOK_INT_LIT) */
} Token;

/* token_type_name: return a human-readable string for a TokenType.
 * The returned pointer is a string literal — do not free it. */
const char *token_type_name(TokenType type);

#endif /* MY_COMPILER_TOKEN_H */
