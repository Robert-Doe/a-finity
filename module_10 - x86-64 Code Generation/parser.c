/* parser.c — Recursive-descent parser for my_compiler
 * Module 06: Parser
 *
 * Grammar (simplified):
 *   program    = func_def*
 *   func_def   = "int"|"void" IDENT "(" params ")" block
 *   params     = epsilon | "int" IDENT ("," "int" IDENT)*
 *   block      = "{" stmt* "}"
 *   stmt       = var_decl | return_stmt | if_stmt | while_stmt
 *              | expr_stmt | block
 *   var_decl   = "int" IDENT ("=" expr)? ";"
 *   return_stmt= "return" expr ";"
 *   if_stmt    = "if" "(" expr ")" stmt ("else" stmt)?
 *   while_stmt = "while" "(" expr ")" stmt
 *   expr_stmt  = expr ";"
 *   expr       = assign
 *   assign     = IDENT "=" assign | logical_or
 *   logical_or = logical_and ("||" logical_and)*
 *   logical_and= equality ("&&" equality)*
 *   equality   = relational (("=="| "!=") relational)*
 *   relational = add (("<"|">"|"<="|">=") add)*
 *   add        = mul (("+"|"-") mul)*
 *   mul        = unary (("*"|"/"|"%") unary)*
 *   unary      = "-" unary | primary
 *   primary    = INT_LIT | IDENT "(" args ")" | IDENT | "(" expr ")"
 */
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                      */
/* ------------------------------------------------------------------ */

/* Peek at the current token without consuming it. */
static Token cur(Parser *p) {
    return p->lx->current;
}

/* Consume and return the current token. */
static Token advance(Parser *p) {
    return lexer_next(p->lx);
}

/* Check if the current token has type t. */
static int check(Parser *p, TokenType t) {
    return cur(p).type == t;
}

/* Consume if type matches; otherwise emit an error and return 0. */
static int expect(Parser *p, TokenType t, const char *what) {
    if (check(p, t)) {
        advance(p);
        return 1;
    }
    fprintf(stderr, "parse error at line %d: expected %s\n",
            cur(p).line, what);
    p->had_error = 1;
    return 0;
}

/* Consume and return token if type matches, else error sentinel. */
static Token expect_tok(Parser *p, TokenType t, const char *what) {
    if (check(p, t)) return advance(p);
    fprintf(stderr, "parse error at line %d: expected %s\n",
            cur(p).line, what);
    p->had_error = 1;
    Token dummy; memset(&dummy, 0, sizeof(dummy)); dummy.type = TOK_ERROR;
    return dummy;
}

/* ------------------------------------------------------------------ */
/* Forward declarations                                                  */
/* ------------------------------------------------------------------ */

static Node *parse_expr(Parser *p);
static Node *parse_stmt(Parser *p);
static Node *parse_block(Parser *p);

/* ------------------------------------------------------------------ */
/* Expressions                                                           */
/* ------------------------------------------------------------------ */

static Node *parse_args(Parser *p, Node *call) {
    /* args already allocated, nargs == 0 */
    if (check(p, TOK_RPAREN)) return call; /* no args */
    do {
        Node *arg = parse_expr(p);
        call->args = realloc(call->args, sizeof(Node*) * (size_t)(call->nargs + 1));
        call->args[call->nargs++] = arg;
    } while (!check(p, TOK_EOF) && !check(p, TOK_RPAREN) &&
             (check(p, TOK_COMMA) ? (advance(p), 1) : 0));
    return call;
}

static Node *parse_primary(Parser *p) {
    Token t = cur(p);

    if (t.type == TOK_INT_LIT) {
        advance(p);
        Node *n = node_new(AST_INT_LIT, t.line);
        n->ival = t.ival;
        return n;
    }

    if (t.type == TOK_IDENT) {
        advance(p);
        if (check(p, TOK_LPAREN)) {
            /* function call */
            advance(p);
            Node *n = node_new(AST_CALL, t.line);
            n->sval = strdup(t.text);
            parse_args(p, n);
            expect(p, TOK_RPAREN, "')'");
            return n;
        }
        /* plain identifier */
        Node *n = node_new(AST_IDENT, t.line);
        n->sval = strdup(t.text);
        return n;
    }

    if (t.type == TOK_LPAREN) {
        advance(p);
        Node *n = parse_expr(p);
        expect(p, TOK_RPAREN, "')'");
        return n;
    }

    fprintf(stderr, "parse error at line %d: unexpected token in expression\n", t.line);
    p->had_error = 1;
    advance(p);
    return node_new(AST_INT_LIT, t.line); /* error recovery */
}

static Node *parse_unary(Parser *p) {
    Token t = cur(p);
    if (t.type == TOK_MINUS) {
        advance(p);
        Node *n = node_new(AST_UNARY, t.line);
        n->op   = '-';
        n->left = parse_unary(p);
        return n;
    }
    return parse_primary(p);
}

static Node *parse_mul(Parser *p) {
    Node *n = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        Token op = advance(p);
        Node *r  = parse_unary(p);
        Node *b  = node_new(AST_BINARY, op.line);
        b->op    = (op.type == TOK_STAR) ? '*' : (op.type == TOK_SLASH) ? '/' : '%';
        b->left  = n;
        b->right = r;
        n = b;
    }
    return n;
}

static Node *parse_add(Parser *p) {
    Node *n = parse_mul(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        Token op = advance(p);
        Node *r  = parse_mul(p);
        Node *b  = node_new(AST_BINARY, op.line);
        b->op    = (op.type == TOK_PLUS) ? '+' : '-';
        b->left  = n;
        b->right = r;
        n = b;
    }
    return n;
}

static Node *parse_relational(Parser *p) {
    Node *n = parse_add(p);
    while (check(p, TOK_LT) || check(p, TOK_GT) ||
           check(p, TOK_LEQ) || check(p, TOK_GEQ)) {
        Token op = advance(p);
        Node *r  = parse_add(p);
        Node *b  = node_new(AST_BINARY, op.line);
        b->op    = (op.type == TOK_LT)  ? '<' :
                   (op.type == TOK_GT)  ? '>' :
                   (op.type == TOK_LEQ) ? 'L' : 'G'; /* L=<=, G=>= */
        b->left  = n;
        b->right = r;
        n = b;
    }
    return n;
}

static Node *parse_equality(Parser *p) {
    Node *n = parse_relational(p);
    while (check(p, TOK_EQEQ) || check(p, TOK_NEQ)) {
        Token op = advance(p);
        Node *r  = parse_relational(p);
        Node *b  = node_new(AST_BINARY, op.line);
        b->op    = (op.type == TOK_EQEQ) ? 'E' : 'N'; /* E===, N=!= */
        b->left  = n;
        b->right = r;
        n = b;
    }
    return n;
}

static Node *parse_logical_and(Parser *p) {
    Node *n = parse_equality(p);
    while (check(p, TOK_AND)) {
        Token op = advance(p);
        Node *r  = parse_equality(p);
        Node *b  = node_new(AST_BINARY, op.line);
        b->op    = '&';
        b->left  = n;
        b->right = r;
        n = b;
    }
    return n;
}

static Node *parse_logical_or(Parser *p) {
    Node *n = parse_logical_and(p);
    while (check(p, TOK_OR)) {
        Token op = advance(p);
        Node *r  = parse_logical_and(p);
        Node *b  = node_new(AST_BINARY, op.line);
        b->op    = '|';
        b->left  = n;
        b->right = r;
        n = b;
    }
    return n;
}

/* Full expression entry. */
static Node *parse_expr(Parser *p) {
    /* Parse the expression via parse_logical_or (which handles all precedence
     * levels via the call chain).  After parsing, if the result is a plain
     * identifier and the next token is '=', we wrap it in an AST_ASSIGN node.
     * This gives us correct two-token-lookahead assignment handling without
     * a separate parse_assign function. */
    Node *n = parse_logical_or(p);
    if (n->kind == AST_IDENT && check(p, TOK_EQ)) {
        advance(p); /* consume '=' */
        Node *rhs = parse_expr(p);
        Node *a   = node_new(AST_ASSIGN, n->line);
        a->left   = n;
        a->right  = rhs;
        return a;
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* Statements                                                            */
/* ------------------------------------------------------------------ */

static Node *parse_var_decl(Parser *p) {
    Token kw = advance(p); /* consume 'int' */
    Token id = expect_tok(p, TOK_IDENT, "variable name");
    Node *n  = node_new(AST_VAR_DECL, kw.line);
    n->sval  = strdup(id.text);
    if (check(p, TOK_EQ)) {
        advance(p);
        n->left = parse_expr(p);
    }
    expect(p, TOK_SEMICOLON, "';'");
    return n;
}

static Node *parse_return(Parser *p) {
    Token kw = advance(p); /* consume 'return' */
    Node *n  = node_new(AST_RETURN, kw.line);
    if (!check(p, TOK_SEMICOLON))
        n->left = parse_expr(p);
    expect(p, TOK_SEMICOLON, "';'");
    return n;
}

static Node *parse_if(Parser *p) {
    Token kw = advance(p); /* consume 'if' */
    expect(p, TOK_LPAREN, "'('");
    Node *cond = parse_expr(p);
    expect(p, TOK_RPAREN, "')'");
    Node *then = parse_stmt(p);
    Node *n    = node_new(AST_IF, kw.line);
    n->left    = cond;
    n->right   = then;
    if (check(p, TOK_ELSE)) {
        advance(p);
        n->extra = parse_stmt(p);
    }
    return n;
}

static Node *parse_while(Parser *p) {
    Token kw = advance(p); /* consume 'while' */
    expect(p, TOK_LPAREN, "'('");
    Node *cond = parse_expr(p);
    expect(p, TOK_RPAREN, "')'");
    Node *body = parse_stmt(p);
    Node *n    = node_new(AST_WHILE, kw.line);
    n->left    = cond;
    n->right   = body;
    return n;
}

static Node *parse_block(Parser *p) {
    Token lb = expect_tok(p, TOK_LBRACE, "'{'");
    Node *n  = node_new(AST_BLOCK, lb.line);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Node *s = parse_stmt(p);
        n->args = realloc(n->args, sizeof(Node*) * (size_t)(n->nargs + 1));
        n->args[n->nargs++] = s;
    }
    expect(p, TOK_RBRACE, "'}'");
    return n;
}

static Node *parse_stmt(Parser *p) {
    if (check(p, TOK_INT))    return parse_var_decl(p);
    if (check(p, TOK_RETURN)) return parse_return(p);
    if (check(p, TOK_IF))     return parse_if(p);
    if (check(p, TOK_WHILE))  return parse_while(p);
    if (check(p, TOK_LBRACE)) return parse_block(p);

    /* expression statement */
    Node *e  = parse_expr(p);
    Node *es = node_new(AST_EXPR_STMT, e->line);
    es->left = e;
    expect(p, TOK_SEMICOLON, "';'");
    return es;
}

/* ------------------------------------------------------------------ */
/* Function definition                                                   */
/* ------------------------------------------------------------------ */

static Node *parse_func(Parser *p) {
    Token ret_kw = advance(p); /* 'int' or 'void' */
    Token name   = expect_tok(p, TOK_IDENT, "function name");
    Node *n      = node_new(AST_FUNC, ret_kw.line);
    n->sval      = strdup(name.text);

    expect(p, TOK_LPAREN, "'('");
    /* Parse parameters: (void) or (int a, int b, ...) */
    if (!check(p, TOK_RPAREN)) {
        if (check(p, TOK_VOID)) {
            advance(p); /* consume 'void' */
        } else {
            /* parse int IDENT pairs */
            do {
                expect(p, TOK_INT, "'int'");
                Token pname = expect_tok(p, TOK_IDENT, "parameter name");
                Node *param = node_new(AST_VAR_DECL, pname.line);
                param->sval = strdup(pname.text);
                n->args = realloc(n->args, sizeof(Node*) * (size_t)(n->nargs + 1));
                n->args[n->nargs++] = param;
            } while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF) &&
                     (check(p, TOK_COMMA) ? (advance(p), 1) : 0));
        }
    }
    expect(p, TOK_RPAREN, "')'");

    n->extra = parse_block(p);
    return n;
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

void parser_init(Parser *p, Lexer *lx) {
    p->lx        = lx;
    p->had_error = 0;
}

Node *parse_program(Parser *p) {
    Node *prog = node_new(AST_PROGRAM, 1);
    while (!check(p, TOK_EOF)) {
        if (!check(p, TOK_INT) && !check(p, TOK_VOID)) {
            fprintf(stderr, "parse error at line %d: expected function definition\n",
                    cur(p).line);
            p->had_error = 1;
            advance(p);
            continue;
        }
        Node *fn = parse_func(p);
        prog->args = realloc(prog->args, sizeof(Node*) * (size_t)(prog->nargs + 1));
        prog->args[prog->nargs++] = fn;
    }
    if (p->had_error) { node_free(prog); return NULL; }
    return prog;
}
