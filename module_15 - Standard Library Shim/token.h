/* token.h — Token types and Token struct for my_compiler
 * Module 15: Standard Library Shim (adds print keyword)
 * Prerequisite: source.h
 */
#ifndef MY_COMPILER_TOKEN_H
#define MY_COMPILER_TOKEN_H

/* Every distinct kind of token the lexer can produce. */
typedef enum {
    /* Literals */
    TOK_INT_LIT,        /* integer literal: 42, 0, 123 */

    /* Identifiers and keywords */
    TOK_IDENT,          /* identifier: foo, x, myVar */
    TOK_KW_INT,         /* keyword: int */
    TOK_KW_VOID,        /* keyword: void */
    TOK_KW_RETURN,      /* keyword: return */
    TOK_KW_IF,          /* keyword: if */
    TOK_KW_ELSE,        /* keyword: else */
    TOK_KW_WHILE,       /* keyword: while */
    TOK_KW_PRINT,       /* keyword: print  (Module 15 addition) */

    /* Arithmetic operators */
    TOK_PLUS,           /* + */
    TOK_MINUS,          /* - */
    TOK_STAR,           /* * */
    TOK_SLASH,          /* / */
    TOK_PERCENT,        /* % */

    /* Comparison operators */
    TOK_EQ,             /* == */
    TOK_NEQ,            /* != */
    TOK_LT,             /* < */
    TOK_GT,             /* > */
    TOK_LEQ,            /* <= */
    TOK_GEQ,            /* >= */

    /* Assignment */
    TOK_ASSIGN,         /* = */

    /* Logical operators */
    TOK_BANG,           /* ! */
    TOK_AMPAMP,         /* && */
    TOK_PIPEPIPE,       /* || */

    /* Punctuation */
    TOK_LPAREN,         /* ( */
    TOK_RPAREN,         /* ) */
    TOK_LBRACE,         /* { */
    TOK_RBRACE,         /* } */
    TOK_SEMICOLON,      /* ; */
    TOK_COMMA,          /* , */

    /* Sentinel */
    TOK_EOF,            /* end of input */
    TOK_ERROR           /* unrecognised character */
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

/* Return a human-readable name for a token type (for error messages). */
static inline const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_INT_LIT:    return "INT_LIT";
        case TOK_IDENT:      return "IDENT";
        case TOK_KW_INT:     return "KW_int";
        case TOK_KW_VOID:    return "KW_void";
        case TOK_KW_RETURN:  return "KW_return";
        case TOK_KW_IF:      return "KW_if";
        case TOK_KW_ELSE:    return "KW_else";
        case TOK_KW_WHILE:   return "KW_while";
        case TOK_KW_PRINT:   return "KW_print";
        case TOK_PLUS:       return "PLUS";
        case TOK_MINUS:      return "MINUS";
        case TOK_STAR:       return "STAR";
        case TOK_SLASH:      return "SLASH";
        case TOK_PERCENT:    return "PERCENT";
        case TOK_EQ:         return "EQ";
        case TOK_NEQ:        return "NEQ";
        case TOK_LT:         return "LT";
        case TOK_GT:         return "GT";
        case TOK_LEQ:        return "LEQ";
        case TOK_GEQ:        return "GEQ";
        case TOK_ASSIGN:     return "ASSIGN";
        case TOK_BANG:       return "BANG";
        case TOK_AMPAMP:     return "AMPAMP";
        case TOK_PIPEPIPE:   return "PIPEPIPE";
        case TOK_LPAREN:     return "LPAREN";
        case TOK_RPAREN:     return "RPAREN";
        case TOK_LBRACE:     return "LBRACE";
        case TOK_RBRACE:     return "RBRACE";
        case TOK_SEMICOLON:  return "SEMICOLON";
        case TOK_COMMA:      return "COMMA";
        case TOK_EOF:        return "EOF";
        case TOK_ERROR:      return "ERROR";
        default:             return "?";
    }
}

#endif /* MY_COMPILER_TOKEN_H */
