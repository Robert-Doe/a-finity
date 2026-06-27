/*
 * ast.h — Abstract Syntax Tree for mycc
 * Module 16: The Complete Compiler
 */
#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef enum {
    /* Expressions */
    AST_INT_LIT,     /* integer literal */
    AST_IDENT,       /* variable reference */
    AST_BINOP,       /* binary operation */
    AST_UNOP,        /* unary operation */
    AST_ASSIGN,      /* assignment  lhs = rhs */
    AST_CALL,        /* function call */

    /* Statements */
    AST_RETURN,      /* return expr; */
    AST_IF,          /* if (cond) then [else alt] */
    AST_WHILE,       /* while (cond) body */
    AST_BLOCK,       /* { stmts... } */
    AST_EXPR_STMT,   /* expr; */
    AST_VAR_DECL,    /* int name; */
    AST_PRINT,       /* print(expr); */

    /* Top-level */
    AST_FUNC,        /* function definition */
    AST_PROGRAM      /* list of functions */
} NodeKind;

typedef struct ASTNode ASTNode;

struct ASTNode {
    NodeKind  kind;
    int       line;

    union {
        /* AST_INT_LIT */
        long ival;

        /* AST_IDENT, AST_VAR_DECL */
        char *name;

        /* AST_BINOP, AST_UNOP */
        struct {
            int      op;   /* token type used as operator */
            ASTNode *left;
            ASTNode *right; /* NULL for unary */
        } binop;

        /* AST_ASSIGN */
        struct {
            char    *name;
            ASTNode *value;
        } assign;

        /* AST_CALL */
        struct {
            char     *name;
            ASTNode **args;
            int       argc;
        } call;

        /* AST_RETURN, AST_EXPR_STMT, AST_PRINT */
        ASTNode *expr;

        /* AST_IF */
        struct {
            ASTNode *cond;
            ASTNode *then_branch;
            ASTNode *else_branch; /* may be NULL */
        } if_stmt;

        /* AST_WHILE */
        struct {
            ASTNode *cond;
            ASTNode *body;
        } while_stmt;

        /* AST_BLOCK */
        struct {
            ASTNode **stmts;
            int       count;
        } block;

        /* AST_FUNC */
        struct {
            char     *name;
            char    **params;
            int       param_count;
            ASTNode  *body;
        } func;

        /* AST_PROGRAM */
        struct {
            ASTNode **funcs;
            int       count;
        } program;
    } u;
};

ASTNode *ast_new(NodeKind kind, int line);
void     ast_free(ASTNode *node);
void     ast_print(const ASTNode *node, int indent);

#endif /* AST_H */
