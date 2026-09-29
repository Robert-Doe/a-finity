/* token.h — Token types and Token struct for my_compiler
 * Module 02: Lexer
 */
#ifndef MY_COMPILER_TOKEN_H
#define MY_COMPILER_TOKEN_H

typedef enum {
    TOK_INT_LIT,
    TOK_IDENT,
    TOK_INT,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_VOID,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_LT,
    TOK_GT,
    TOK_LEQ,
    TOK_GEQ,
    TOK_EQEQ,
    TOK_NEQ,
    TOK_AND,
    TOK_OR,
    TOK_EQ,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType   type;
    int         line;
    int         col;
    long        ival;
    char        text[64];
} Token;

#endif /* MY_COMPILER_TOKEN_H */
