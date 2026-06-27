/* token.h — Token types and Token struct for bob_compiler
 * Module 02: Lexer
 * Prerequisite: source.h
 */
#ifndef BOB_TOKEN_H
#define BOB_TOKEN_H

typedef enum {
    /* Literals */
    TOK_INT_LIT,

    /* Identifiers and keywords */
    TOK_IDENT,
    TOK_KW_INT,    /* keyword: int  */
    TOK_KW_VOID,   /* keyword: void */
    TOK_KW_RETURN, /* keyword: return */
    TOK_KW_IF,     /* keyword: if */
    TOK_KW_ELSE,   /* keyword: else */
    TOK_KW_WHILE,  /* keyword: while */

    /* Arithmetic */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,

    /* Comparison */
    TOK_LT,
    TOK_GT,
    TOK_LEQ,
    TOK_GEQ,
    TOK_EQ,   /* == */
    TOK_NEQ,  /* != */

    /* Logical */
    TOK_AMPAMP,   /* && */
    TOK_PIPEPIPE, /* || */

    /* Assignment and unary */
    TOK_ASSIGN, /* = */
    TOK_BANG,   /* ! */

    /* Punctuation */
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_COMMA,

    /* Sentinels */
    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    int       line;
    int       col;
    long      ival;      /* TOK_INT_LIT: numeric value */
    char      text[64];  /* TOK_IDENT: identifier text */
} Token;

#endif /* BOB_TOKEN_H */
