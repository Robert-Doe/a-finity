/*
 * parser.c — Recursive-descent parser
 * Module 11: Code Generation — Control Flow
 *
 * Grammar (simplified):
 *   program   = func*
 *   func      = 'int'|'void' IDENT '(' params ')' block
 *   params    = 'void' | (type IDENT (',' type IDENT)*)
 *   block     = '{' stmt* '}'
 *   stmt      = var_decl | if_stmt | while_stmt | return_stmt | expr_stmt
 *   var_decl  = 'int' IDENT ';'
 *   if_stmt   = 'if' '(' expr ')' block ('else' block)?
 *   while_stmt= 'while' '(' expr ')' block
 *   return_stmt = 'return' expr? ';'
 *   expr_stmt = expr ';'
 *   expr      = assign
 *   assign    = IDENT '=' assign | or_expr
 *   or_expr   = and_expr ('||' and_expr)*
 *   and_expr  = cmp_expr ('&&' cmp_expr)*
 *   cmp_expr  = add_expr (('<'|'>'|'<='|'>='|'=='|'!=') add_expr)?
 *   add_expr  = mul_expr (('+' | '-') mul_expr)*
 *   mul_expr  = unary (('*' | '/' | '%') unary)*
 *   unary     = '-' unary | '!' unary | primary
 *   primary   = INT_LIT | IDENT '(' args ')' | IDENT | '(' expr ')'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* ------------------------------------------------------------- utilities */

static void advance(Parser *p) {
    p->cur = lexer_next(&p->lex);
}

static Token peek(Parser *p) {
    return lexer_peek(&p->lex);
}

static int check(Parser *p, TokenType t) {
    return p->cur.type == t;
}

static int match(Parser *p, TokenType t) {
    if (p->cur.type == t) { advance(p); return 1; }
    return 0;
}

static Token expect(Parser *p, TokenType t) {
    if (p->cur.type != t) {
        fprintf(stderr, "parse error at line %d: expected %s, got %s\n",
                p->cur.line, token_type_name(t), token_type_name(p->cur.type));
        p->errors++;
    }
    Token tok = p->cur;
    advance(p);
    return tok;
}

static char *tok_str(Token t) {
    char *s = malloc(t.len + 1);
    memcpy(s, t.start, t.len);
    s[t.len] = '\0';
    return s;
}

/* ---------------------------------------------------------- forward decls */

static Node *parse_expr(Parser *p);
static Node *parse_stmt(Parser *p);
static Node *parse_block(Parser *p);

/* ------------------------------------------------------------- expressions */

static Node *parse_primary(Parser *p) {
    Token t = p->cur;
    if (t.type == TOK_INT_LIT) {
        advance(p);
        Node *n = ast_node(AST_INT_LIT, t.line);
        n->ival = t.ival;
        return n;
    }
    if (t.type == TOK_IDENT) {
        advance(p);
        /* function call? */
        if (check(p, TOK_LPAREN)) {
            advance(p); /* consume '(' */
            Node *n = ast_node(AST_CALL, t.line);
            n->sval = tok_str(t);
            /* parse arguments */
            int cap = 8;
            n->args  = malloc(sizeof(Node*) * (size_t)cap);
            n->nargs = 0;
            while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
                if (n->nargs >= cap) {
                    cap *= 2;
                    n->args = realloc(n->args, sizeof(Node*) * (size_t)cap);
                }
                n->args[n->nargs++] = parse_expr(p);
                if (!match(p, TOK_COMMA)) break;
            }
            expect(p, TOK_RPAREN);
            return n;
        }
        Node *n = ast_node(AST_IDENT, t.line);
        n->sval = tok_str(t);
        return n;
    }
    if (t.type == TOK_LPAREN) {
        advance(p);
        Node *n = parse_expr(p);
        expect(p, TOK_RPAREN);
        return n;
    }
    fprintf(stderr, "parse error at line %d: unexpected token %s in expression\n",
            t.line, token_type_name(t.type));
    p->errors++;
    advance(p);
    return ast_node(AST_INT_LIT, t.line); /* error recovery: return 0 */
}

static Node *parse_unary(Parser *p) {
    Token t = p->cur;
    if (t.type == TOK_MINUS || t.type == TOK_BANG) {
        advance(p);
        Node *n = ast_node(AST_UNARY, t.line);
        n->op   = (t.type == TOK_MINUS) ? '-' : '!';
        n->left = parse_unary(p);
        return n;
    }
    return parse_primary(p);
}

static Node *parse_mul(Parser *p) {
    Node *left = parse_unary(p);
    for (;;) {
        char op = 0;
        if (p->cur.type == TOK_STAR)    op = '*';
        else if (p->cur.type == TOK_SLASH)   op = '/';
        else if (p->cur.type == TOK_PERCENT) op = '%';
        else break;
        int line = p->cur.line;
        advance(p);
        Node *n  = ast_node(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = parse_unary(p);
        left = n;
    }
    return left;
}

static Node *parse_add(Parser *p) {
    Node *left = parse_mul(p);
    for (;;) {
        char op = 0;
        if (p->cur.type == TOK_PLUS)  op = '+';
        else if (p->cur.type == TOK_MINUS) op = '-';
        else break;
        int line = p->cur.line;
        advance(p);
        Node *n  = ast_node(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = parse_mul(p);
        left = n;
    }
    return left;
}

static Node *parse_cmp(Parser *p) {
    Node *left = parse_add(p);
    /* single comparison operator */
    char op = 0; int line = p->cur.line;
    TokenType tt = p->cur.type;
    if      (tt == TOK_LT)  op = '<';
    else if (tt == TOK_GT)  op = '>';
    else if (tt == TOK_LEQ) op = 'L'; /* <= */
    else if (tt == TOK_GEQ) op = 'G'; /* >= */
    else if (tt == TOK_EQ)  op = 'E'; /* == */
    else if (tt == TOK_NEQ) op = 'N'; /* != */
    if (op) {
        advance(p);
        Node *n  = ast_node(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = parse_add(p);
        return n;
    }
    return left;
}

static Node *parse_and(Parser *p) {
    Node *left = parse_cmp(p);
    while (p->cur.type == TOK_AMPAMP) {
        int line = p->cur.line;
        advance(p);
        Node *n  = ast_node(AST_BINARY, line);
        n->op    = '&';
        n->left  = left;
        n->right = parse_cmp(p);
        left = n;
    }
    return left;
}

static Node *parse_or(Parser *p) {
    Node *left = parse_and(p);
    while (p->cur.type == TOK_PIPEPIPE) {
        int line = p->cur.line;
        advance(p);
        Node *n  = ast_node(AST_BINARY, line);
        n->op    = '|';
        n->left  = left;
        n->right = parse_and(p);
        left = n;
    }
    return left;
}

static Node *parse_assign(Parser *p) {
    /* Look-ahead: IDENT '=' => assignment */
    if (p->cur.type == TOK_IDENT && peek(p).type == TOK_ASSIGN) {
        Token name = p->cur;
        advance(p); /* consume IDENT */
        advance(p); /* consume '=' */
        Node *n   = ast_node(AST_ASSIGN, name.line);
        n->sval   = tok_str(name);
        n->right  = parse_assign(p);
        return n;
    }
    return parse_or(p);
}

static Node *parse_expr(Parser *p) {
    return parse_assign(p);
}

/* ------------------------------------------------------------- statements */

static Node *parse_block(Parser *p) {
    Token lbrace = p->cur;
    expect(p, TOK_LBRACE);
    Node *blk  = ast_node(AST_BLOCK, lbrace.line);
    int cap    = 16;
    blk->args  = malloc(sizeof(Node*) * (size_t)cap);
    blk->nargs = 0;

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (blk->nargs >= cap) {
            cap *= 2;
            blk->args = realloc(blk->args, sizeof(Node*) * (size_t)cap);
        }
        blk->args[blk->nargs++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE);
    return blk;
}

static Node *parse_stmt(Parser *p) {
    Token t = p->cur;

    /* var declaration: int IDENT ; */
    if (t.type == TOK_KW_INT && peek(p).type == TOK_IDENT) {
        advance(p); /* consume 'int' */
        Token name = p->cur;
        advance(p); /* consume IDENT */
        Node *n  = ast_node(AST_VAR_DECL, t.line);
        n->sval  = tok_str(name);
        n->right = NULL;
        /* optional initializer: = expr */
        if (match(p, TOK_ASSIGN)) {
            n->right = parse_expr(p);
        }
        expect(p, TOK_SEMICOLON);
        return n;
    }

    /* if statement */
    if (t.type == TOK_KW_IF) {
        advance(p);
        Node *n = ast_node(AST_IF, t.line);
        expect(p, TOK_LPAREN);
        n->left  = parse_expr(p);
        expect(p, TOK_RPAREN);
        n->right = parse_block(p);
        if (check(p, TOK_KW_ELSE)) {
            advance(p);
            n->extra = parse_block(p);
        }
        return n;
    }

    /* while statement */
    if (t.type == TOK_KW_WHILE) {
        advance(p);
        Node *n = ast_node(AST_WHILE, t.line);
        expect(p, TOK_LPAREN);
        n->left  = parse_expr(p);
        expect(p, TOK_RPAREN);
        n->right = parse_block(p);
        return n;
    }

    /* return statement */
    if (t.type == TOK_KW_RETURN) {
        advance(p);
        Node *n = ast_node(AST_RETURN, t.line);
        if (!check(p, TOK_SEMICOLON)) {
            n->left = parse_expr(p);
        }
        expect(p, TOK_SEMICOLON);
        return n;
    }

    /* expression statement */
    {
        Node *n  = ast_node(AST_EXPR_STMT, t.line);
        n->left  = parse_expr(p);
        expect(p, TOK_SEMICOLON);
        return n;
    }
}

/* ------------------------------------------------------------- functions / top level */

static Node *parse_func(Parser *p) {
    Token ret_type = p->cur;
    /* consume return type: 'int' or 'void' */
    if (ret_type.type != TOK_KW_INT && ret_type.type != TOK_KW_VOID) {
        fprintf(stderr, "parse error at line %d: expected 'int' or 'void'\n", ret_type.line);
        p->errors++;
    }
    advance(p);

    Token name = expect(p, TOK_IDENT);
    Node *fn   = ast_node(AST_FUNC, name.line);
    fn->sval   = tok_str(name);

    expect(p, TOK_LPAREN);

    int cap    = 8;
    fn->args   = malloc(sizeof(Node*) * (size_t)cap);
    fn->nargs  = 0;

    /* Parameters: 'void' means no params */
    if (check(p, TOK_KW_VOID) && peek(p).type == TOK_RPAREN) {
        advance(p); /* consume 'void' */
    } else if (!check(p, TOK_RPAREN)) {
        for (;;) {
            expect(p, TOK_KW_INT); /* only 'int' params */
            Token pname = expect(p, TOK_IDENT);
            Node *pnode = ast_node(AST_IDENT, pname.line);
            pnode->sval = tok_str(pname);
            if (fn->nargs >= cap) {
                cap *= 2;
                fn->args = realloc(fn->args, sizeof(Node*) * (size_t)cap);
            }
            fn->args[fn->nargs++] = pnode;
            if (!match(p, TOK_COMMA)) break;
        }
    }

    expect(p, TOK_RPAREN);
    fn->right = parse_block(p);
    return fn;
}

/* ------------------------------------------------------------- public API */

void parser_init(Parser *p, const char *src) {
    lexer_init(&p->lex, src);
    p->errors = 0;
    advance(p); /* prime the look-ahead */
}

Node *parser_parse(Parser *p) {
    Node *prog  = ast_node(AST_PROGRAM, 1);
    int   cap   = 16;
    prog->args  = malloc(sizeof(Node*) * (size_t)cap);
    prog->nargs = 0;

    while (!check(p, TOK_EOF)) {
        if (prog->nargs >= cap) {
            cap *= 2;
            prog->args = realloc(prog->args, sizeof(Node*) * (size_t)cap);
        }
        prog->args[prog->nargs++] = parse_func(p);
    }
    return prog;
}
