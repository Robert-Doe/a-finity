/*
 * ast.c — AST node allocation and utilities for mycc
 * Module 16: The Complete Compiler
 */
#include "ast.h"
#include "token.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ASTNode *ast_new(NodeKind kind, int line)
{
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!n) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    n->kind = kind;
    n->line = line;
    return n;
}

void ast_free(ASTNode *node)
{
    if (!node) return;
    switch (node->kind) {
        case AST_IDENT:
        case AST_VAR_DECL:
            free(node->u.name);
            break;
        case AST_BINOP:
        case AST_UNOP:
            ast_free(node->u.binop.left);
            ast_free(node->u.binop.right);
            break;
        case AST_ASSIGN:
            free(node->u.assign.name);
            ast_free(node->u.assign.value);
            break;
        case AST_CALL:
            free(node->u.call.name);
            for (int i = 0; i < node->u.call.argc; i++)
                ast_free(node->u.call.args[i]);
            free(node->u.call.args);
            break;
        case AST_RETURN:
        case AST_EXPR_STMT:
        case AST_PRINT:
            ast_free(node->u.expr);
            break;
        case AST_IF:
            ast_free(node->u.if_stmt.cond);
            ast_free(node->u.if_stmt.then_branch);
            ast_free(node->u.if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->u.while_stmt.cond);
            ast_free(node->u.while_stmt.body);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->u.block.count; i++)
                ast_free(node->u.block.stmts[i]);
            free(node->u.block.stmts);
            break;
        case AST_FUNC:
            free(node->u.func.name);
            for (int i = 0; i < node->u.func.param_count; i++)
                free(node->u.func.params[i]);
            free(node->u.func.params);
            ast_free(node->u.func.body);
            break;
        case AST_PROGRAM:
            for (int i = 0; i < node->u.program.count; i++)
                ast_free(node->u.program.funcs[i]);
            free(node->u.program.funcs);
            break;
        default:
            break;
    }
    free(node);
}

static void indent_print(int depth)
{
    for (int i = 0; i < depth * 2; i++) putchar(' ');
}

void ast_print(const ASTNode *node, int indent)
{
    if (!node) return;
    indent_print(indent);
    switch (node->kind) {
        case AST_INT_LIT:
            printf("INT(%ld)\n", node->u.ival);
            break;
        case AST_IDENT:
            printf("IDENT(%s)\n", node->u.name);
            break;
        case AST_BINOP:
            printf("BINOP(%s)\n", token_type_name((TokenType)node->u.binop.op));
            ast_print(node->u.binop.left,  indent+1);
            ast_print(node->u.binop.right, indent+1);
            break;
        case AST_UNOP:
            printf("UNOP(%s)\n", token_type_name((TokenType)node->u.binop.op));
            ast_print(node->u.binop.left, indent+1);
            break;
        case AST_ASSIGN:
            printf("ASSIGN(%s)\n", node->u.assign.name);
            ast_print(node->u.assign.value, indent+1);
            break;
        case AST_CALL:
            printf("CALL(%s, argc=%d)\n", node->u.call.name, node->u.call.argc);
            for (int i = 0; i < node->u.call.argc; i++)
                ast_print(node->u.call.args[i], indent+1);
            break;
        case AST_RETURN:
            printf("RETURN\n");
            ast_print(node->u.expr, indent+1);
            break;
        case AST_PRINT:
            printf("PRINT\n");
            ast_print(node->u.expr, indent+1);
            break;
        case AST_EXPR_STMT:
            printf("EXPR_STMT\n");
            ast_print(node->u.expr, indent+1);
            break;
        case AST_IF:
            printf("IF\n");
            ast_print(node->u.if_stmt.cond,        indent+1);
            ast_print(node->u.if_stmt.then_branch,  indent+1);
            ast_print(node->u.if_stmt.else_branch,  indent+1);
            break;
        case AST_WHILE:
            printf("WHILE\n");
            ast_print(node->u.while_stmt.cond, indent+1);
            ast_print(node->u.while_stmt.body, indent+1);
            break;
        case AST_BLOCK:
            printf("BLOCK(%d)\n", node->u.block.count);
            for (int i = 0; i < node->u.block.count; i++)
                ast_print(node->u.block.stmts[i], indent+1);
            break;
        case AST_VAR_DECL:
            printf("VAR_DECL(%s)\n", node->u.name);
            break;
        case AST_FUNC:
            printf("FUNC(%s, params=%d)\n", node->u.func.name, node->u.func.param_count);
            ast_print(node->u.func.body, indent+1);
            break;
        case AST_PROGRAM:
            printf("PROGRAM(%d funcs)\n", node->u.program.count);
            for (int i = 0; i < node->u.program.count; i++)
                ast_print(node->u.program.funcs[i], indent+1);
            break;
        default:
            printf("UNKNOWN_NODE(%d)\n", node->kind);
    }
}
