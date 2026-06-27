/*
 * parser.c — Full recursive-descent parser (Module 06, unchanged)
 *
 * Grammar (simplified):
 *
 *   program      → func_def*  EOF
 *   func_def     → type IDENT '(' param_list ')' block
 *   param_list   → 'void' | (type IDENT (',' type IDENT)*)  | ε
 *   type         → 'int'
 *   block        → '{' stmt* '}'
 *   stmt         → var_decl | return_stmt | if_stmt | while_stmt
 *                | expr_stmt
 *   var_decl     → 'int' IDENT ';'
 *   return_stmt  → 'return' expr? ';'
 *   if_stmt      → 'if' '(' expr ')' block ('else' block)?
 *   while_stmt   → 'while' '(' expr ')' block
 *   expr_stmt    → expr ';'
 *   expr         → assignment
 *   assignment   → IDENT '=' assignment | logic_or
 *   logic_or     → logic_and ('||' logic_and)*
 *   logic_and    → equality ('&&' equality)*
 *   equality     → relational (('=='|'!=') relational)*
 *   relational   → additive (('<'|'>'|'<='|'>=') additive)*
 *   additive     → multiplicative (('+'|'-') multiplicative)*
 *   multiplicative → unary (('*'|'/'|'%') unary)*
 *   unary        → ('-'|'!') unary | primary
 *   primary      → INT_LIT | IDENT ('(' arg_list ')')? | '(' expr ')'
 *   arg_list     → (expr (',' expr)*)?
 */

#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Low-level helpers                                                   */
/* ------------------------------------------------------------------ */

static void error_at(Parser *p, const Token *t, const char *msg)
{
    fprintf(stderr, "parse error at line %d col %d: %s (got '%.*s')\n",
            t->line, t->col, msg, (int)t->len, t->start);
    p->had_error = 1;
}

/* Advance to the next token, saving the old one in p->previous.     */
static Token advance(Parser *p)
{
    p->previous = p->current;
    p->current  = lexer_next(p->lexer);
    return p->previous;
}

/* Return 1 if the current token is of the given type.               */
static int check(const Parser *p, TokenType t)
{
    return p->current.type == t;
}

/* If current token matches t, consume and return 1; else return 0.  */
static int match(Parser *p, TokenType t)
{
    if (!check(p, t)) return 0;
    advance(p);
    return 1;
}

/* Consume a token of type t or report an error.                     */
static Token expect(Parser *p, TokenType t, const char *msg)
{
    if (check(p, t)) return advance(p);
    error_at(p, &p->current, msg);
    return p->current;   /* return something so callers don't crash  */
}

/* Copy a token's lexeme into a new heap string.                     */
static char *tok_strdup(const Token *t)
{
    char *s = malloc(t->len + 1);
    if (!s) { fprintf(stderr, "parser: out of memory\n"); exit(1); }
    memcpy(s, t->start, t->len);
    s[t->len] = '\0';
    return s;
}

/* ------------------------------------------------------------------ */
/* Forward declarations for mutual recursion                          */
/* ------------------------------------------------------------------ */
static Node *parse_expr(Parser *p);
static Node *parse_stmt(Parser *p);
static Node *parse_block(Parser *p);

/* ------------------------------------------------------------------ */
/* Expressions                                                         */
/* ------------------------------------------------------------------ */

/* primary → INT_LIT | IDENT ('(' arg_list ')')? | '(' expr ')' */
static Node *parse_primary(Parser *p)
{
    /* Integer literal */
    if (match(p, TOK_INT_LIT)) {
        Node *n = node_new(AST_INT_LIT, p->previous.line);
        n->ival = p->previous.ival;
        return n;
    }

    /* Identifier — possibly a function call */
    if (match(p, TOK_IDENT)) {
        Token name_tok = p->previous;

        if (match(p, TOK_LPAREN)) {
            /* function call */
            Node *call = node_new(AST_CALL, name_tok.line);
            call->sval = tok_strdup(&name_tok);

            /* arg_list */
            int   cap  = 8;
            Node **arr = malloc(sizeof(Node *) * (size_t)cap);
            if (!arr) { fprintf(stderr, "parser: OOM\n"); exit(1); }
            int cnt = 0;

            if (!check(p, TOK_RPAREN)) {
                do {
                    if (cnt == cap) {
                        cap *= 2;
                        arr  = realloc(arr, sizeof(Node *) * (size_t)cap);
                        if (!arr) { fprintf(stderr, "parser: OOM\n"); exit(1); }
                    }
                    arr[cnt++] = parse_expr(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, "expected ')' after arguments");
            call->args  = arr;
            call->nargs = cnt;
            return call;
        }

        /* Plain identifier */
        Node *n  = node_new(AST_IDENT, name_tok.line);
        n->sval  = tok_strdup(&name_tok);
        return n;
    }

    /* Grouped expression */
    if (match(p, TOK_LPAREN)) {
        Node *inner = parse_expr(p);
        expect(p, TOK_RPAREN, "expected ')' after grouped expression");
        return inner;
    }

    /* Nothing matched */
    error_at(p, &p->current, "expected expression");
    /* Return a dummy node so the caller doesn't get NULL. */
    return node_new(AST_INT_LIT, p->current.line);
}

/* unary → ('-'|'!') unary | primary */
static Node *parse_unary(Parser *p)
{
    if (match(p, TOK_MINUS) || match(p, TOK_BANG)) {
        char op  = (p->previous.type == TOK_MINUS) ? '-' : '!';
        int  ln  = p->previous.line;
        Node *n  = node_new(AST_UNARY, ln);
        n->op    = op;
        n->left  = parse_unary(p);
        return n;
    }
    return parse_primary(p);
}

/* Helper for left-associative binary levels.
 * kinds[] is a 0-terminated array of (TokenType, op_char) pairs.    */
typedef struct { TokenType tok; char op; } OpEntry;

static Node *parse_binary_level(Parser *p,
                                 Node *(*sub)(Parser *),
                                 const OpEntry *ops)
{
    Node *left = sub(p);
    for (;;) {
        char op = 0;
        for (const OpEntry *e = ops; e->tok != TOK_EOF; e++) {
            if (check(p, e->tok)) { op = e->op; break; }
        }
        if (!op) break;
        int ln = p->current.line;
        advance(p);
        Node *n  = node_new(AST_BINARY, ln);
        n->op    = op;
        n->left  = left;
        n->right = sub(p);
        left     = n;
    }
    return left;
}

static Node *parse_multiplicative(Parser *p)
{
    static const OpEntry ops[] = {
        { TOK_STAR, '*' }, { TOK_SLASH, '/' }, { TOK_PERCENT, '%' },
        { TOK_EOF,  0   }
    };
    return parse_binary_level(p, parse_unary, ops);
}

static Node *parse_additive(Parser *p)
{
    static const OpEntry ops[] = {
        { TOK_PLUS, '+' }, { TOK_MINUS, '-' }, { TOK_EOF, 0 }
    };
    return parse_binary_level(p, parse_multiplicative, ops);
}

static Node *parse_relational(Parser *p)
{
    /* We need multi-char ops; use a manual loop. */
    Node *left = parse_additive(p);
    for (;;) {
        char op = 0;
        if      (match(p, TOK_LT))  op = '<';
        else if (match(p, TOK_GT))  op = '>';
        else if (match(p, TOK_LEQ)) op = 'L';   /* <= */
        else if (match(p, TOK_GEQ)) op = 'G';   /* >= */
        else break;
        int   ln = p->previous.line;
        Node *n  = node_new(AST_BINARY, ln);
        n->op    = op;
        n->left  = left;
        n->right = parse_additive(p);
        left     = n;
    }
    return left;
}

static Node *parse_equality(Parser *p)
{
    Node *left = parse_relational(p);
    for (;;) {
        char op = 0;
        if      (match(p, TOK_EQ))  op = '=';   /* == */
        else if (match(p, TOK_NEQ)) op = 'N';   /* != */
        else break;
        int   ln = p->previous.line;
        Node *n  = node_new(AST_BINARY, ln);
        n->op    = op;
        n->left  = left;
        n->right = parse_relational(p);
        left     = n;
    }
    return left;
}

static Node *parse_logic_and(Parser *p)
{
    Node *left = parse_equality(p);
    while (match(p, TOK_AMPAMP)) {
        int   ln = p->previous.line;
        Node *n  = node_new(AST_BINARY, ln);
        n->op    = '&';
        n->left  = left;
        n->right = parse_equality(p);
        left     = n;
    }
    return left;
}

static Node *parse_logic_or(Parser *p)
{
    Node *left = parse_logic_and(p);
    while (match(p, TOK_PIPEPIPE)) {
        int   ln = p->previous.line;
        Node *n  = node_new(AST_BINARY, ln);
        n->op    = '|';
        n->left  = left;
        n->right = parse_logic_and(p);
        left     = n;
    }
    return left;
}

/* assignment → IDENT '=' assignment | logic_or */
static Node *parse_assignment(Parser *p)
{
    /* We need to look ahead: if we see IDENT followed by '=', it's
     * an assignment.  Otherwise fall through to logic_or.           */
    if (check(p, TOK_IDENT)) {
        /* Peek: is the token after the ident a '='?                 */
        Token saved_cur  = p->current;
        Token saved_prev = p->previous;
        advance(p);   /* consume the IDENT into p->previous          */
        Token ident_tok = p->previous;

        if (match(p, TOK_ASSIGN)) {
            /* Yes: it's an assignment.                              */
            Node *lhs  = node_new(AST_IDENT, ident_tok.line);
            lhs->sval  = tok_strdup(&ident_tok);
            Node *n    = node_new(AST_ASSIGN, ident_tok.line);
            n->left    = lhs;
            n->right   = parse_assignment(p);   /* right-associative */
            return n;
        }

        /* Not an assignment — we consumed the IDENT already.
         * Reconstruct a primary node and continue up through the
         * binary-expression levels.                                 */
        (void)saved_cur;
        (void)saved_prev;

        Node *ident_node = node_new(AST_IDENT, ident_tok.line);
        ident_node->sval = tok_strdup(&ident_tok);

        /* Check for function call immediately after ident.         */
        if (match(p, TOK_LPAREN)) {
            Node *call = node_new(AST_CALL, ident_tok.line);
            call->sval = ident_node->sval;
            ident_node->sval = NULL;
            node_free(ident_node);

            int   cap  = 8;
            Node **arr = malloc(sizeof(Node *) * (size_t)cap);
            if (!arr) { fprintf(stderr, "parser: OOM\n"); exit(1); }
            int cnt = 0;

            if (!check(p, TOK_RPAREN)) {
                do {
                    if (cnt == cap) {
                        cap *= 2;
                        arr  = realloc(arr, sizeof(Node *) * (size_t)cap);
                        if (!arr) { fprintf(stderr, "parser: OOM\n"); exit(1); }
                    }
                    arr[cnt++] = parse_expr(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, "expected ')' after arguments");
            call->args  = arr;
            call->nargs = cnt;
            ident_node  = call;
        }

        /* Now climb the binary levels manually starting from our
         * already-parsed primary (ident_node).                      */

        /* multiplicative */
        Node *left = ident_node;
        for (;;) {
            char op = 0;
            if      (check(p, TOK_STAR))    op = '*';
            else if (check(p, TOK_SLASH))   op = '/';
            else if (check(p, TOK_PERCENT)) op = '%';
            if (!op) break;
            int ln = p->current.line; advance(p);
            Node *n = node_new(AST_BINARY, ln);
            n->op = op; n->left = left; n->right = parse_unary(p);
            left = n;
        }
        /* additive */
        for (;;) {
            char op = 0;
            if      (check(p, TOK_PLUS))  op = '+';
            else if (check(p, TOK_MINUS)) op = '-';
            if (!op) break;
            int ln = p->current.line; advance(p);
            Node *n = node_new(AST_BINARY, ln);
            n->op = op; n->left = left; n->right = parse_multiplicative(p);
            left = n;
        }
        /* relational */
        for (;;) {
            char op = 0;
            if      (match(p, TOK_LT))  op = '<';
            else if (match(p, TOK_GT))  op = '>';
            else if (match(p, TOK_LEQ)) op = 'L';
            else if (match(p, TOK_GEQ)) op = 'G';
            else break;
            int ln = p->previous.line;
            Node *n = node_new(AST_BINARY, ln);
            n->op = op; n->left = left; n->right = parse_additive(p);
            left = n;
        }
        /* equality */
        for (;;) {
            char op = 0;
            if      (match(p, TOK_EQ))  op = '=';
            else if (match(p, TOK_NEQ)) op = 'N';
            else break;
            int ln = p->previous.line;
            Node *n = node_new(AST_BINARY, ln);
            n->op = op; n->left = left; n->right = parse_relational(p);
            left = n;
        }
        /* logic_and */
        while (match(p, TOK_AMPAMP)) {
            int ln = p->previous.line;
            Node *n = node_new(AST_BINARY, ln);
            n->op = '&'; n->left = left; n->right = parse_equality(p);
            left = n;
        }
        /* logic_or */
        while (match(p, TOK_PIPEPIPE)) {
            int ln = p->previous.line;
            Node *n = node_new(AST_BINARY, ln);
            n->op = '|'; n->left = left; n->right = parse_logic_and(p);
            left = n;
        }
        return left;
    }

    return parse_logic_or(p);
}

static Node *parse_expr(Parser *p)
{
    return parse_assignment(p);
}

/* ------------------------------------------------------------------ */
/* Statements                                                          */
/* ------------------------------------------------------------------ */

/* block → '{' stmt* '}' */
static Node *parse_block(Parser *p)
{
    int ln = p->current.line;
    expect(p, TOK_LBRACE, "expected '{'");

    int    cap  = 16;
    Node **arr  = malloc(sizeof(Node *) * (size_t)cap);
    if (!arr) { fprintf(stderr, "parser: OOM\n"); exit(1); }
    int cnt = 0;

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (cnt == cap) {
            cap *= 2;
            arr  = realloc(arr, sizeof(Node *) * (size_t)cap);
            if (!arr) { fprintf(stderr, "parser: OOM\n"); exit(1); }
        }
        arr[cnt++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE, "expected '}'");

    Node *block = node_new(AST_BLOCK, ln);
    block->args  = arr;
    block->nargs = cnt;
    return block;
}

/* stmt → var_decl | return_stmt | if_stmt | while_stmt | expr_stmt */
static Node *parse_stmt(Parser *p)
{
    /* var_decl: 'int' IDENT ';' */
    if (match(p, TOK_KW_INT)) {
        int ln = p->previous.line;
        Token name = expect(p, TOK_IDENT, "expected variable name");
        expect(p, TOK_SEMICOLON, "expected ';' after variable declaration");
        Node *d  = node_new(AST_VAR_DECL, ln);
        d->sval  = tok_strdup(&name);
        return d;
    }

    /* return_stmt: 'return' expr? ';' */
    if (match(p, TOK_KW_RETURN)) {
        int ln   = p->previous.line;
        Node *r  = node_new(AST_RETURN, ln);
        if (!check(p, TOK_SEMICOLON))
            r->left = parse_expr(p);
        expect(p, TOK_SEMICOLON, "expected ';' after return");
        return r;
    }

    /* if_stmt */
    if (match(p, TOK_KW_IF)) {
        int ln = p->previous.line;
        expect(p, TOK_LPAREN, "expected '(' after 'if'");
        Node *cond = parse_expr(p);
        expect(p, TOK_RPAREN, "expected ')' after condition");
        Node *then_block = parse_block(p);
        Node *else_block = NULL;
        if (match(p, TOK_KW_ELSE))
            else_block = parse_block(p);
        Node *n  = node_new(AST_IF, ln);
        n->left  = cond;
        n->right = then_block;
        n->extra = else_block;
        return n;
    }

    /* while_stmt */
    if (match(p, TOK_KW_WHILE)) {
        int ln = p->previous.line;
        expect(p, TOK_LPAREN, "expected '(' after 'while'");
        Node *cond = parse_expr(p);
        expect(p, TOK_RPAREN, "expected ')' after condition");
        Node *body = parse_block(p);
        Node *n    = node_new(AST_WHILE, ln);
        n->left    = cond;
        n->right   = body;
        return n;
    }

    /* expr_stmt */
    {
        int   ln = p->current.line;
        Node *e  = parse_expr(p);
        expect(p, TOK_SEMICOLON, "expected ';' after expression");
        Node *s  = node_new(AST_EXPR_STMT, ln);
        s->left  = e;
        return s;
    }
}

/* ------------------------------------------------------------------ */
/* Function definition                                                 */
/* ------------------------------------------------------------------ */

/* type → 'int' | 'void'  (we just consume it; in our language every
 *                          return type is int or void but we treat
 *                          everything uniformly in the AST)          */
static int parse_type(Parser *p)
{
    if (match(p, TOK_KW_INT))  return 1;
    if (match(p, TOK_KW_VOID)) return 1;
    error_at(p, &p->current, "expected type keyword (int/void)");
    return 0;
}

/* func_def → type IDENT '(' param_list ')' block */
static Node *parse_func(Parser *p)
{
    int ln = p->current.line;
    parse_type(p);   /* consume return type */

    Token name = expect(p, TOK_IDENT, "expected function name");
    expect(p, TOK_LPAREN, "expected '(' after function name");

    /* param_list: 'void' or comma-separated 'int' IDENT pairs, or ε */
    int    cap    = 8;
    Node **params = malloc(sizeof(Node *) * (size_t)cap);
    if (!params) { fprintf(stderr, "parser: OOM\n"); exit(1); }
    int nparams = 0;

    if (match(p, TOK_KW_VOID)) {
        /* explicit void param list — no parameters */
    } else if (!check(p, TOK_RPAREN)) {
        /* one or more parameters */
        do {
            expect(p, TOK_KW_INT, "expected 'int' in parameter list");
            Token pname = expect(p, TOK_IDENT, "expected parameter name");
            if (nparams == cap) {
                cap *= 2;
                params = realloc(params, sizeof(Node *) * (size_t)cap);
                if (!params) { fprintf(stderr, "parser: OOM\n"); exit(1); }
            }
            Node *param = node_new(AST_VAR_DECL, pname.line);
            param->sval = tok_strdup(&pname);
            params[nparams++] = param;
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN, "expected ')' after parameter list");

    Node *body = parse_block(p);

    Node *func   = node_new(AST_FUNC, ln);
    func->sval   = tok_strdup(&name);
    func->args   = params;
    func->nargs  = nparams;
    func->extra  = body;
    return func;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void parser_init(Parser *p, Lexer *lx)
{
    p->lexer         = lx;
    p->had_error     = 0;
    /* Initialise previous to a harmless sentinel.                   */
    p->previous.type = TOK_EOF;
    p->previous.start = "";
    p->previous.len   = 0;
    p->previous.line  = 0;
    p->previous.col   = 0;
    p->previous.ival  = 0;
    /* Prime the lookahead with the very first real token.           */
    p->current = lexer_next(lx);
}

Node *parse_program(Parser *p)
{
    int    cap   = 16;
    Node **funcs = malloc(sizeof(Node *) * (size_t)cap);
    if (!funcs) { fprintf(stderr, "parser: OOM\n"); exit(1); }
    int nfuncs = 0;

    while (!check(p, TOK_EOF)) {
        if (nfuncs == cap) {
            cap *= 2;
            funcs = realloc(funcs, sizeof(Node *) * (size_t)cap);
            if (!funcs) { fprintf(stderr, "parser: OOM\n"); exit(1); }
        }
        funcs[nfuncs++] = parse_func(p);
    }

    Node *prog  = node_new(AST_PROGRAM, 1);
    prog->args  = funcs;
    prog->nargs = nfuncs;
    return prog;
}
