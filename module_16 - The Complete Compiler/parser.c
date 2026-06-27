/*
 * parser.c — Recursive-descent parser for mycc
 * Module 16: The Complete Compiler
 *
 * Grammar (simplified):
 *   program    := func*
 *   func       := type IDENT '(' params ')' block
 *   params     := ε | type IDENT (',' type IDENT)*
 *   block      := '{' stmt* '}'
 *   stmt       := var_decl | return_stmt | if_stmt | while_stmt
 *               | print_stmt | expr_stmt
 *   expr_stmt  := expr ';'
 *   expr       := assign
 *   assign     := IDENT '=' assign | or_expr
 *   or_expr    := and_expr ('||' and_expr)*
 *   and_expr   := eq_expr  ('&&' eq_expr)*
 *   eq_expr    := rel_expr (('=='|'!=') rel_expr)*
 *   rel_expr   := add_expr (('<'|'<='|'>'|'>=') add_expr)*
 *   add_expr   := mul_expr (('+' | '-') mul_expr)*
 *   mul_expr   := unary   (('*'|'/'|'%') unary)*
 *   unary      := '-' unary | '!' unary | primary
 *   primary    := INT_LIT | IDENT | IDENT '(' args ')' | '(' expr ')'
 */
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ helpers */

static void error(Parser *p, const char *msg)
{
    fprintf(stderr, "%d:%d: parse error: %s (got '%.*s')\n",
            p->cur.line, p->cur.col, msg,
            (int)p->cur.len, p->cur.start);
    p->errors++;
}

static Token advance(Parser *p)
{
    Token prev = p->cur;
    p->cur = lexer_next(p->lex);
    return prev;
}

static int check(Parser *p, TokenType t)
{
    return p->cur.type == t;
}

static int match(Parser *p, TokenType t)
{
    if (!check(p, t)) return 0;
    advance(p);
    return 1;
}

static Token expect(Parser *p, TokenType t)
{
    if (!check(p, t)) {
        char buf[64];
        snprintf(buf, sizeof buf, "expected '%s'", token_type_name(t));
        error(p, buf);
    }
    return advance(p);
}

/* ----------------------------------------------------------------- forward decls */
static ASTNode *parse_stmt(Parser *p);
static ASTNode *parse_expr(Parser *p);
static ASTNode *parse_block(Parser *p);

/* ----------------------------------------------------------------- expressions */

static char *dup_tok(const Token *t)
{
    char *s = (char *)malloc(t->len + 1);
    if (!s) { fprintf(stderr, "parser: oom\n"); exit(1); }
    memcpy(s, t->start, t->len);
    s[t->len] = '\0';
    return s;
}

static ASTNode *parse_primary(Parser *p)
{
    int line = p->cur.line;

    if (check(p, TOK_INT_LIT)) {
        Token t = advance(p);
        ASTNode *n = ast_new(AST_INT_LIT, line);
        n->u.ival = t.ival;
        return n;
    }

    if (check(p, TOK_IDENT)) {
        Token t = advance(p);
        /* function call? */
        if (check(p, TOK_LPAREN)) {
            advance(p); /* eat ( */
            ASTNode *n = ast_new(AST_CALL, line);
            n->u.call.name = dup_tok(&t);
            n->u.call.args = NULL;
            n->u.call.argc = 0;
            int cap = 0;
            if (!check(p, TOK_RPAREN)) {
                do {
                    if (n->u.call.argc == cap) {
                        cap = cap ? cap * 2 : 4;
                        n->u.call.args = realloc(n->u.call.args, (size_t)cap * sizeof(ASTNode*));
                    }
                    n->u.call.args[n->u.call.argc++] = parse_expr(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN);
            return n;
        }
        /* plain identifier */
        ASTNode *n = ast_new(AST_IDENT, line);
        n->u.name = dup_tok(&t);
        return n;
    }

    if (match(p, TOK_LPAREN)) {
        ASTNode *inner = parse_expr(p);
        expect(p, TOK_RPAREN);
        return inner;
    }

    error(p, "expected expression");
    /* return a dummy node so parsing can continue */
    ASTNode *dummy = ast_new(AST_INT_LIT, line);
    dummy->u.ival = 0;
    return dummy;
}

static ASTNode *parse_unary(Parser *p)
{
    int line = p->cur.line;
    if (check(p, TOK_MINUS) || check(p, TOK_NOT)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_UNOP, line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = parse_unary(p);
        n->u.binop.right = NULL;
        return n;
    }
    return parse_primary(p);
}

static ASTNode *parse_mul(Parser *p)
{
    ASTNode *left = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_BINOP, left->line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = left;
        n->u.binop.right = parse_unary(p);
        left = n;
    }
    return left;
}

static ASTNode *parse_add(Parser *p)
{
    ASTNode *left = parse_mul(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_BINOP, left->line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = left;
        n->u.binop.right = parse_mul(p);
        left = n;
    }
    return left;
}

static ASTNode *parse_rel(Parser *p)
{
    ASTNode *left = parse_add(p);
    while (check(p, TOK_LT) || check(p, TOK_LE) ||
           check(p, TOK_GT) || check(p, TOK_GE)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_BINOP, left->line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = left;
        n->u.binop.right = parse_add(p);
        left = n;
    }
    return left;
}

static ASTNode *parse_eq(Parser *p)
{
    ASTNode *left = parse_rel(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_BINOP, left->line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = left;
        n->u.binop.right = parse_rel(p);
        left = n;
    }
    return left;
}

static ASTNode *parse_and(Parser *p)
{
    ASTNode *left = parse_eq(p);
    while (check(p, TOK_AND)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_BINOP, left->line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = left;
        n->u.binop.right = parse_eq(p);
        left = n;
    }
    return left;
}

static ASTNode *parse_or(Parser *p)
{
    ASTNode *left = parse_and(p);
    while (check(p, TOK_OR)) {
        Token op = advance(p);
        ASTNode *n = ast_new(AST_BINOP, left->line);
        n->u.binop.op    = (int)op.type;
        n->u.binop.left  = left;
        n->u.binop.right = parse_and(p);
        left = n;
    }
    return left;
}

static ASTNode *parse_assign(Parser *p)
{
    /* Peek: if IDENT followed by '=', it's assignment */
    if (check(p, TOK_IDENT)) {
        Lexer saved = *p->lex;
        Token cur_saved = p->cur;

        Token ident = advance(p);
        if (check(p, TOK_ASSIGN)) {
            advance(p); /* eat = */
            ASTNode *n = ast_new(AST_ASSIGN, ident.line);
            n->u.assign.name  = dup_tok(&ident);
            n->u.assign.value = parse_assign(p);
            return n;
        }
        /* Not assignment — restore and parse as or_expr */
        *p->lex = saved;
        p->cur  = cur_saved;
    }
    return parse_or(p);
}

static ASTNode *parse_expr(Parser *p)
{
    return parse_assign(p);
}

/* ----------------------------------------------------------------- statements */

static ASTNode *parse_stmt(Parser *p)
{
    int line = p->cur.line;

    /* var declaration: int name; */
    if (check(p, TOK_KW_INT)) {
        advance(p);
        Token name = expect(p, TOK_IDENT);
        expect(p, TOK_SEMICOLON);
        ASTNode *n = ast_new(AST_VAR_DECL, line);
        n->u.name = dup_tok(&name);
        return n;
    }

    /* return */
    if (match(p, TOK_KW_RETURN)) {
        ASTNode *n = ast_new(AST_RETURN, line);
        if (check(p, TOK_SEMICOLON)) {
            advance(p);
            n->u.expr = NULL;
        } else {
            n->u.expr = parse_expr(p);
            expect(p, TOK_SEMICOLON);
        }
        return n;
    }

    /* if */
    if (match(p, TOK_KW_IF)) {
        expect(p, TOK_LPAREN);
        ASTNode *cond = parse_expr(p);
        expect(p, TOK_RPAREN);
        ASTNode *then_b = parse_stmt(p);
        ASTNode *else_b = NULL;
        if (match(p, TOK_KW_ELSE))
            else_b = parse_stmt(p);
        ASTNode *n = ast_new(AST_IF, line);
        n->u.if_stmt.cond        = cond;
        n->u.if_stmt.then_branch = then_b;
        n->u.if_stmt.else_branch = else_b;
        return n;
    }

    /* while */
    if (match(p, TOK_KW_WHILE)) {
        expect(p, TOK_LPAREN);
        ASTNode *cond = parse_expr(p);
        expect(p, TOK_RPAREN);
        ASTNode *body = parse_stmt(p);
        ASTNode *n = ast_new(AST_WHILE, line);
        n->u.while_stmt.cond = cond;
        n->u.while_stmt.body = body;
        return n;
    }

    /* print(expr); */
    if (match(p, TOK_KW_PRINT)) {
        expect(p, TOK_LPAREN);
        ASTNode *val = parse_expr(p);
        expect(p, TOK_RPAREN);
        expect(p, TOK_SEMICOLON);
        ASTNode *n = ast_new(AST_PRINT, line);
        n->u.expr = val;
        return n;
    }

    /* block */
    if (check(p, TOK_LBRACE))
        return parse_block(p);

    /* expression statement */
    ASTNode *expr = parse_expr(p);
    expect(p, TOK_SEMICOLON);
    ASTNode *n = ast_new(AST_EXPR_STMT, line);
    n->u.expr = expr;
    return n;
}

static ASTNode *parse_block(Parser *p)
{
    int line = p->cur.line;
    expect(p, TOK_LBRACE);
    ASTNode *n = ast_new(AST_BLOCK, line);
    n->u.block.stmts = NULL;
    n->u.block.count = 0;
    int cap = 0;
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (n->u.block.count == cap) {
            cap = cap ? cap * 2 : 8;
            n->u.block.stmts = realloc(n->u.block.stmts,
                                       (size_t)cap * sizeof(ASTNode*));
        }
        n->u.block.stmts[n->u.block.count++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE);
    return n;
}

/* ----------------------------------------------------------------- functions / program */

static int is_type(Parser *p)
{
    return check(p, TOK_KW_INT) || check(p, TOK_KW_VOID);
}

static ASTNode *parse_func(Parser *p)
{
    int line = p->cur.line;
    /* return type */
    if (!is_type(p)) { error(p, "expected type"); return NULL; }
    advance(p);

    Token name = expect(p, TOK_IDENT);
    expect(p, TOK_LPAREN);

    char **params = NULL;
    int param_count = 0;
    int param_cap   = 0;

    if (!check(p, TOK_RPAREN)) {
        do {
            if (!is_type(p)) { error(p, "expected parameter type"); break; }
            advance(p); /* eat type */
            Token pname = expect(p, TOK_IDENT);
            if (param_count == param_cap) {
                param_cap = param_cap ? param_cap * 2 : 4;
                params = realloc(params, (size_t)param_cap * sizeof(char*));
            }
            params[param_count++] = dup_tok(&pname);
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN);

    ASTNode *body = parse_block(p);

    ASTNode *n = ast_new(AST_FUNC, line);
    n->u.func.name        = dup_tok(&name);
    n->u.func.params      = params;
    n->u.func.param_count = param_count;
    n->u.func.body        = body;
    return n;
}

void parser_init(Parser *p, Lexer *lex)
{
    p->lex    = lex;
    p->errors = 0;
    p->cur    = lexer_next(lex);
}

ASTNode *parse_program(Parser *p)
{
    ASTNode *prog = ast_new(AST_PROGRAM, 1);
    prog->u.program.funcs = NULL;
    prog->u.program.count = 0;
    int cap = 0;

    while (!check(p, TOK_EOF)) {
        ASTNode *fn = parse_func(p);
        if (!fn) break;
        if (prog->u.program.count == cap) {
            cap = cap ? cap * 2 : 4;
            prog->u.program.funcs = realloc(prog->u.program.funcs,
                                            (size_t)cap * sizeof(ASTNode*));
        }
        prog->u.program.funcs[prog->u.program.count++] = fn;
    }
    return prog;
}
