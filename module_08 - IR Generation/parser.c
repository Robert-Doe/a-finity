/* parser.c — Recursive-descent parser for my_compiler
 * Module 04-06: Parser
 */
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                               */
/* ------------------------------------------------------------------ */

/* Peek at the current token without consuming it. */
static Token peek(Parser *p) {
    return p->lx->current;
}

/* Consume and return the current token. */
static Token advance(Parser *p) {
    return lexer_next(p->lx);
}

/* Consume a token of the expected type or report an error. */
static Token expect(Parser *p, TokenType type, const char *what) {
    if (p->lx->current.type != type) {
        fprintf(stderr, "line %d: expected %s\n", p->lx->current.line, what);
        p->had_error = 1;
    }
    return advance(p);
}

/* Check whether the current token matches type. */
static int check(Parser *p, TokenType type) {
    return p->lx->current.type == type;
}

/* Consume if current matches type; return 1 if consumed. */
static int match(Parser *p, TokenType type) {
    if (check(p, type)) { advance(p); return 1; }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Forward declarations for mutual recursion                            */
/* ------------------------------------------------------------------ */
static Node *parse_expr(Parser *p);
static Node *parse_stmt(Parser *p);
static Node *parse_block(Parser *p);

/* ------------------------------------------------------------------ */
/* Expression parsing (Pratt / precedence climbing)                     */
/* ------------------------------------------------------------------ */

static Node *parse_primary(Parser *p) {
    Token t = peek(p);

    if (t.type == TOK_INT_LIT) {
        advance(p);
        Node *n = node_new(AST_INT_LIT, t.line);
        n->ival = t.ival;
        return n;
    }

    if (t.type == TOK_IDENT) {
        advance(p);
        /* Function call */
        if (check(p, TOK_LPAREN)) {
            advance(p); /* consume ( */
            Node *n = node_new(AST_CALL, t.line);
            strncpy(n->name, t.text, sizeof(n->name) - 1);
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
                if (n->n_args > 0) expect(p, TOK_COMMA, "','");
                n->args[n->n_args++] = parse_expr(p);
            }
            expect(p, TOK_RPAREN, "')'");
            return n;
        }
        /* Plain identifier */
        Node *n = node_new(AST_IDENT, t.line);
        strncpy(n->name, t.text, sizeof(n->name) - 1);
        return n;
    }

    if (t.type == TOK_LPAREN) {
        advance(p);
        Node *n = parse_expr(p);
        expect(p, TOK_RPAREN, "')'");
        return n;
    }

    if (t.type == TOK_MINUS) {
        advance(p);
        Node *n = node_new(AST_UNARY, t.line);
        n->op = TOK_MINUS;
        n->args[0] = parse_primary(p);
        n->n_args = 1;
        return n;
    }

    fprintf(stderr, "line %d: unexpected token in expression\n", t.line);
    p->had_error = 1;
    advance(p);
    return node_new(AST_INT_LIT, t.line); /* error recovery: return 0 literal */
}

static int binary_op_prec(TokenType t) {
    switch (t) {
        case TOK_OR:      return 1;
        case TOK_AND:     return 2;
        case TOK_EQEQ: case TOK_NEQ: return 3;
        case TOK_LT: case TOK_GT: case TOK_LEQ: case TOK_GEQ: return 4;
        case TOK_PLUS: case TOK_MINUS: return 5;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 6;
        default: return -1;
    }
}

static Node *parse_binary(Parser *p, int min_prec) {
    Node *left = parse_primary(p);
    for (;;) {
        int prec = binary_op_prec(peek(p).type);
        if (prec < min_prec) break;
        Token op = advance(p);
        Node *right = parse_binary(p, prec + 1);
        Node *n = node_new(AST_BINARY, op.line);
        n->op = op.type;
        n->args[0] = left;
        n->args[1] = right;
        n->n_args  = 2;
        left = n;
    }
    return left;
}

static Node *parse_expr(Parser *p) {
    /* Check for assignment: IDENT = expr */
    if (peek(p).type == TOK_IDENT) {
        Token saved = peek(p);
        /* We need 2-token lookahead; simulate by consuming and checking */
        advance(p); /* consume ident */
        if (check(p, TOK_EQ)) {
            advance(p); /* consume = */
            Node *n = node_new(AST_ASSIGN, saved.line);
            strncpy(n->name, saved.text, sizeof(n->name) - 1);
            n->args[0] = parse_expr(p);
            n->n_args  = 1;
            return n;
        }
        /* Not an assignment — put back by rebuilding the left side */
        /* We already consumed the ident; check if it's a call */
        Node *ident;
        if (check(p, TOK_LPAREN)) {
            advance(p);
            ident = node_new(AST_CALL, saved.line);
            strncpy(ident->name, saved.text, sizeof(ident->name) - 1);
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
                if (ident->n_args > 0) expect(p, TOK_COMMA, "','");
                ident->args[ident->n_args++] = parse_expr(p);
            }
            expect(p, TOK_RPAREN, "')'");
        } else {
            ident = node_new(AST_IDENT, saved.line);
            strncpy(ident->name, saved.text, sizeof(ident->name) - 1);
        }
        /* Now continue binary parsing with ident as left */
        for (;;) {
            int prec = binary_op_prec(peek(p).type);
            if (prec < 1) break;
            Token op = advance(p);
            Node *right = parse_binary(p, prec + 1);
            Node *n = node_new(AST_BINARY, op.line);
            n->op = op.type;
            n->args[0] = ident;
            n->args[1] = right;
            n->n_args  = 2;
            ident = n;
        }
        return ident;
    }
    return parse_binary(p, 1);
}

/* ------------------------------------------------------------------ */
/* Statement parsing                                                     */
/* ------------------------------------------------------------------ */

static Node *parse_block(Parser *p) {
    Token t = peek(p);
    expect(p, TOK_LBRACE, "'{'");
    Node *block = node_new(AST_BLOCK, t.line);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Node *s = parse_stmt(p);
        if (s) block->args[block->n_args++] = s;
    }
    expect(p, TOK_RBRACE, "'}'");
    return block;
}

static Node *parse_stmt(Parser *p) {
    Token t = peek(p);

    /* return expr; */
    if (t.type == TOK_RETURN) {
        advance(p);
        Node *n = node_new(AST_RETURN, t.line);
        if (!check(p, TOK_SEMICOLON)) {
            n->args[0] = parse_expr(p);
            n->n_args  = 1;
        }
        expect(p, TOK_SEMICOLON, "';'");
        return n;
    }

    /* if ( cond ) then [ else alt ] */
    if (t.type == TOK_IF) {
        advance(p);
        Node *n = node_new(AST_IF, t.line);
        expect(p, TOK_LPAREN, "'('");
        n->args[n->n_args++] = parse_expr(p);   /* cond */
        expect(p, TOK_RPAREN, "')'");
        n->args[n->n_args++] = parse_stmt(p);   /* then */
        if (check(p, TOK_ELSE)) {
            advance(p);
            n->args[n->n_args++] = parse_stmt(p); /* else */
        }
        return n;
    }

    /* while ( cond ) body */
    if (t.type == TOK_WHILE) {
        advance(p);
        Node *n = node_new(AST_WHILE, t.line);
        expect(p, TOK_LPAREN, "'('");
        n->args[n->n_args++] = parse_expr(p);   /* cond */
        expect(p, TOK_RPAREN, "')'");
        n->args[n->n_args++] = parse_stmt(p);   /* body */
        return n;
    }

    /* Block */
    if (t.type == TOK_LBRACE) {
        return parse_block(p);
    }

    /* Variable declaration: int name [= expr]; */
    if (t.type == TOK_INT) {
        advance(p);
        Token name = expect(p, TOK_IDENT, "variable name");
        Node *n = node_new(AST_VAR_DECL, t.line);
        strncpy(n->name, name.text, sizeof(n->name) - 1);
        if (check(p, TOK_EQ)) {
            advance(p);
            n->args[n->n_args++] = parse_expr(p);
        }
        expect(p, TOK_SEMICOLON, "';'");
        return n;
    }

    /* Expression statement */
    Node *expr = parse_expr(p);
    Node *stmt = node_new(AST_EXPR_STMT, t.line);
    stmt->args[0] = expr;
    stmt->n_args  = 1;
    expect(p, TOK_SEMICOLON, "';'");
    return stmt;
}

/* ------------------------------------------------------------------ */
/* Top-level parsing                                                     */
/* ------------------------------------------------------------------ */

/* Parse: int name ( params ) { body } */
static Node *parse_function(Parser *p) {
    Token t = peek(p);
    /* Return type: int or void */
    if (!check(p, TOK_INT) && !check(p, TOK_VOID)) {
        fprintf(stderr, "line %d: expected function return type\n", t.line);
        p->had_error = 1;
    }
    advance(p); /* consume type */
    Token name = expect(p, TOK_IDENT, "function name");

    Node *func = node_new(AST_FUNC, t.line);
    strncpy(func->name, name.text, sizeof(func->name) - 1);

    expect(p, TOK_LPAREN, "'('");
    /* Parameter list: void | (int name , ...) */
    if (check(p, TOK_VOID)) {
        advance(p); /* consume void */
    } else {
        while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
            if (func->n_args > 0) expect(p, TOK_COMMA, "','");
            expect(p, TOK_INT, "'int'");
            Token pname = expect(p, TOK_IDENT, "parameter name");
            Node *param = node_new(AST_IDENT, pname.line);
            strncpy(param->name, pname.text, sizeof(param->name) - 1);
            func->args[func->n_args++] = param;
        }
    }
    expect(p, TOK_RPAREN, "')'");

    /* Body block — appended as the last arg */
    Node *body = parse_block(p);
    func->args[func->n_args++] = body;
    return func;
}

Node *parse_program(Parser *p, Lexer *lx) {
    p->lx        = lx;
    p->had_error = 0;

    Node *prog = node_new(AST_PROGRAM, 1);
    while (!check(p, TOK_EOF)) {
        Node *fn = parse_function(p);
        if (fn) prog->args[prog->n_args++] = fn;
    }
    return prog;
}
