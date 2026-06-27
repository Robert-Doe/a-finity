/*
 * token.h — Token type enumeration and Token struct
 *
 * Module: 02 — Lexer / Tokeniser
 * Description: Defines every kind of token the lexer can produce and
 *              the Token struct that carries position and value information.
 *
 * Prerequisites: none (this header has no dependencies).
 */

#ifndef MY_COMPILER_TOKEN_H
#define MY_COMPILER_TOKEN_H

#include <stddef.h>  /* size_t */

/*
 * TokenType — every distinct category of lexeme in our language.
 *
 * Ordering groups: literals, identifiers, keywords, arithmetic operators,
 * comparison operators, assignment/logical operators, punctuation, sentinels.
 */
typedef enum {
    /* --- literals and names --- */
    TOK_INT_LIT,    /* integer literal, e.g. 42 */
    TOK_IDENT,      /* identifier, e.g. result */

    /* --- keywords (reserved identifiers) --- */
    TOK_KW_INT,     /* int    */
    TOK_KW_VOID,    /* void   */
    TOK_KW_RETURN,  /* return */
    TOK_KW_IF,      /* if     */
    TOK_KW_ELSE,    /* else   */
    TOK_KW_WHILE,   /* while  */

    /* --- arithmetic operators --- */
    TOK_PLUS,       /* +  */
    TOK_MINUS,      /* -  */
    TOK_STAR,       /* *  */
    TOK_SLASH,      /* /  */
    TOK_PERCENT,    /* %  */

    /* --- comparison operators --- */
    TOK_EQ,         /* == */
    TOK_NEQ,        /* != */
    TOK_LT,         /* <  */
    TOK_GT,         /* >  */
    TOK_LEQ,        /* <= */
    TOK_GEQ,        /* >= */

    /* --- assignment and logical operators --- */
    TOK_ASSIGN,     /* =  */
    TOK_BANG,       /* !  */
    TOK_AMPAMP,     /* && */
    TOK_PIPEPIPE,   /* || */

    /* --- punctuation --- */
    TOK_LPAREN,     /* (  */
    TOK_RPAREN,     /* )  */
    TOK_LBRACE,     /* {  */
    TOK_RBRACE,     /* }  */
    TOK_SEMICOLON,  /* ;  */
    TOK_COMMA,      /* ,  */

    /* --- sentinels --- */
    TOK_EOF,        /* end of file — the scanning loop terminates on this */
    TOK_ERROR       /* unrecognised character — lets the parser attempt recovery */
} TokenType;

/*
 * Token — one unit of lexical meaning with its source location.
 *
 * Fields:
 *   type  — which kind of token this is.
 *   start — pointer into Source.text where this lexeme begins.
 *           Do NOT free this pointer; it is owned by the Source buffer.
 *   len   — number of bytes in the lexeme (start[0..len-1]).
 *   line  — 1-based source line where the token starts.
 *   col   — 1-based source column where the token starts.
 *   ival  — parsed integer value; only valid when type == TOK_INT_LIT.
 */
typedef struct {
    TokenType   type;
    const char *start;  /* pointer into Source.text — NOT null-terminated on its own */
    size_t      len;
    int         line;   /* 1-based */
    int         col;    /* 1-based */
    long        ival;   /* only valid for TOK_INT_LIT */
} Token;

/*
 * token_type_name — return a human-readable string for a TokenType.
 *
 * type: any value from the TokenType enum.
 * Returns: a static string such as "INT_LIT" or "KW_return".
 *          The returned pointer must not be freed.
 */
const char *token_type_name(TokenType type);

#endif /* MY_COMPILER_TOKEN_H */
