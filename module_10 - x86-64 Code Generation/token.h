/* token.h — Token types and Token struct for my_compiler
 * Module 02: Lexer
 * Prerequisite: source.h
 */
#ifndef MY_COMPILER_TOKEN_H
#define MY_COMPILER_TOKEN_H

/* Every distinct kind of token the lexer can produce. */
typedef enum {
    /* Literals */
    TOK_INT_LIT,    /* integer literal: 42, 0, 123 */

    /* Identifiers and keywords */
    TOK_IDENT,      /* identifier: foo, x, myVar */
    TOK_INT,        /* keyword: int */
    TOK_RETURN,     /* keyword: return */
    TOK_IF,         /* keyword: if */
    TOK_ELSE,       /* keyword: else */
    TOK_WHILE,      /* keyword: while */
    TOK_VOID,       /* keyword: void */

    /* Arithmetic operators */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_STAR,       /* * */
    TOK_SLASH,      /* / */
    TOK_PERCENT,    /* % */

    /* Comparison operators */
    TOK_LT,         /* < */
    TOK_GT,         /* > */
    TOK_LEQ,        /* <= */
    TOK_GEQ,        /* >= */
    TOK_EQEQ,       /* == */
    TOK_NEQ,        /* != */

    /* Logical operators */
    TOK_AND,        /* && */
    TOK_OR,         /* || */

    /* Assignment */
    TOK_EQ,         /* = */

    /* Punctuation */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_LBRACE,     /* { */
    TOK_RBRACE,     /* } */
    TOK_SEMICOLON,  /* ; */
    TOK_COMMA,      /* , */

    /* Sentinel */
    TOK_EOF,        /* end of input */
    TOK_ERROR       /* unrecognised character */
} TokenType;

/* A single token produced by the lexer. */
typedef struct {
    TokenType   type;       /* what kind of token */
    int         line;       /* source line (1-based) */
    int         col;        /* source column (1-based) */
    /* For TOK_INT_LIT: the numeric value */
    long        ival;
    /* For TOK_IDENT: the identifier text (null-terminated) */
    char        text[64];
} Token;

#endif /* MY_COMPILER_TOKEN_H */
