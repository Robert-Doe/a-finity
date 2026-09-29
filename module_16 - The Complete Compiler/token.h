/*
 * token.h — Token types and Token struct for mycc
 * Module 16: The Complete Compiler
 */
#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    /* Literals & identifiers */
    TOK_INT_LIT,       /* 42 */
    TOK_IDENT,         /* foo */

    /* Keywords */
    TOK_KW_INT,        /* int */
    TOK_KW_RETURN,     /* return */
    TOK_KW_IF,         /* if */
    TOK_KW_ELSE,       /* else */
    TOK_KW_WHILE,      /* while */
    TOK_KW_VOID,       /* void */
    TOK_KW_PRINT,      /* print  — built-in print statement */

    /* Punctuation */
    TOK_LPAREN,        /* ( */
    TOK_RPAREN,        /* ) */
    TOK_LBRACE,        /* { */
    TOK_RBRACE,        /* } */
    TOK_SEMICOLON,     /* ; */
    TOK_COMMA,         /* , */

    /* Arithmetic operators */
    TOK_PLUS,          /* + */
    TOK_MINUS,         /* - */
    TOK_STAR,          /* * */
    TOK_SLASH,         /* / */
    TOK_PERCENT,       /* % */

    /* Relational / logical operators */
    TOK_EQ,            /* == */
    TOK_NEQ,           /* != */
    TOK_LT,            /* < */
    TOK_LE,            /* <= */
    TOK_GT,            /* > */
    TOK_GE,            /* >= */

    /* Assignment */
    TOK_ASSIGN,        /* = */

    /* Logical */
    TOK_AND,           /* && */
    TOK_OR,            /* || */
    TOK_NOT,           /* ! */

    /* End of file */
    TOK_EOF
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   /* pointer into source text */
    size_t      len;     /* byte length of lexeme */
    int         line;
    int         col;
    long        ival;    /* filled for TOK_INT_LIT */
} Token;

/* Return a printable name for a token type (for diagnostics). */
const char *token_type_name(TokenType t);

#endif /* TOKEN_H */
