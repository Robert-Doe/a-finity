/* parser.c — Recursive-descent parser for my_compiler
 * Module 06 — adds fully-implemented parse_function and parse_program.
 *             All Module 04/05 parsing functions are included unchanged.
 * Prerequisites: parser.h, lexer.h, ast.h, source.h.
 *
 * Grammar summary (EBNF):
 *   program     = function*
 *   function    = ('int'|'void') IDENT '(' param_list ')' block
 *   param_list  = ( type IDENT (',' type IDENT)* )?
 *   type        = 'int' | 'void'
 *   block       = '{' stmt* '}'
 *   stmt        = var_decl | if_stmt | while_stmt | return_stmt
 *               | IDENT '=' expr ';'
 *               | expr ';'
 *   var_decl    = 'int' IDENT ('=' expr)? ';'
 *   if_stmt     = 'if' '(' expr ')' block ('else' block)?
 *   while_stmt  = 'while' '(' expr ')' block
 *   return_stmt = 'return' expr? ';'
 *   expr        = or_expr
 *   or_expr     = and_expr  ('||' and_expr)*
 *   and_expr    = eq_expr   ('&&' eq_expr)*
 *   eq_expr     = rel_expr  (('=='|'!=') rel_expr)*
 *   rel_expr    = add_expr  (('<'|'>'|'<='|'>=') add_expr)*
 *   add_expr    = mul_expr  (('+'|'-') mul_expr)*
 *   mul_expr    = unary     (('*'|'/'|'%') unary)*
 *   unary       = ('-'|'!') unary | primary
 *   primary     = INT_LIT | IDENT | IDENT '(' arg_list ')' | '(' expr ')'
 *   arg_list    = ( expr (',' expr)* )?
 */
#include "parser.h"

#include <stdio.h>   /* fprintf, printf */
#include <stdlib.h>  /* malloc, realloc, free, exit */
#include <string.h>  /* memcpy, strlen */

/* =========================================================================
 * my_strndup — portable replacement for POSIX strndup.
 *
 * Copies at most 'n' bytes from 's' into a new heap buffer and appends '\0'.
 * The caller owns the returned memory and must free() it.
 * ========================================================================= */
static char *my_strndup(const char *s, size_t n) {
    char *buf = malloc(n + 1);   /* +1 for the NUL terminator */
    if (!buf) {
        fprintf(stderr, "out of memory in my_strndup\n");
        exit(1);
    }
    memcpy(buf, s, n);           /* copy exactly n bytes — may not reach NUL */
    buf[n] = '\0';               /* terminate the copy */
    return buf;
}

/* =========================================================================
 * Token stream management
 * ========================================================================= */

/* advance: consume current, shift next into current, fetch a fresh next.
 * Returns the token that was just consumed (now in p->prev). */
static Token advance(Parser *p) {
    p->prev    = p->current;          /* save the consumed token */
    p->current = p->next;             /* slide the lookahead window forward */
    p->next    = lexer_next(&p->lex); /* fetch one more token from the lexer */
    return p->prev;
}

/* check: return 1 if current token has the given type. */
static int check(const Parser *p, TokenType type) {
    return p->current.type == type;
}

/* match: if current token matches 'type', consume it and return 1; else 0. */
static int match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance(p);
        return 1;
    }
    return 0;
}

/* expect: consume current token if it matches 'type', else report an error.
 * Always returns the consumed (or current) token so callers can inspect it. */
static Token expect(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) {
        return advance(p);           /* happy path: right token */
    }
    /* Error: report and set the error flag, but keep going. */
    fprintf(stderr, "parse error at line %d: expected %s, got '%.*s' (%s)\n",
            p->current.line,
            msg,
            (int)p->current.len,
            p->current.start,
            token_type_name(p->current.type));
    p->had_error = 1;
    return p->current;               /* return the mismatched token */
}

/* =========================================================================
 * parser_init
 * ========================================================================= */

/* parser_init: set up the parser to scan *src.
 * Primes both the current and next lookahead slots so the parser can always
 * look one token ahead without special-casing the very first call. */
void parser_init(Parser *p, const Source *src) {
    lexer_init(&p->lex, src);          /* attach the lexer to the source */
    p->had_error = 0;                  /* no errors yet */
    p->current   = lexer_next(&p->lex); /* fill current with first token */
    p->next      = lexer_next(&p->lex); /* fill next with second token */
    /* prev is unset at this point; it should never be read before advance() */
}

/* =========================================================================
 * Expression parsing  (Modules 04/05 — unchanged here)
 * ========================================================================= */

/* Forward declarations for the mutually recursive expression functions. */
static Node *parse_or(Parser *p);
static Node *parse_and(Parser *p);
static Node *parse_eq(Parser *p);
static Node *parse_rel(Parser *p);
static Node *parse_add(Parser *p);
static Node *parse_mul(Parser *p);
static Node *parse_unary(Parser *p);
static Node *parse_primary(Parser *p);

/* parse_expr: entry point for expression parsing.
 * Delegates to the lowest-precedence level (logical OR). */
Node *parse_expr(Parser *p) {
    return parse_or(p);   /* or_expr is the root of the precedence hierarchy */
}

/* parse_or: logical OR — lowest precedence binary operator.
 * or_expr = and_expr ('||' and_expr)* */
static Node *parse_or(Parser *p) {
    Node *left = parse_and(p);           /* parse the left operand */
    while (check(p, TOK_PIPEPIPE)) {
        int line = p->current.line;      /* record operator position */
        advance(p);                      /* consume '||' */
        Node *right = parse_and(p);     /* parse the right operand */
        Node *n  = node_new(AST_BINARY, line);
        n->op    = 'O';                  /* 'O' encodes || */
        n->left  = left;
        n->right = right;
        left = n;                        /* left-associate: result becomes new left */
    }
    return left;
}

/* parse_and: logical AND.
 * and_expr = eq_expr ('&&' eq_expr)* */
static Node *parse_and(Parser *p) {
    Node *left = parse_eq(p);
    while (check(p, TOK_AMPAMP)) {
        int line = p->current.line;
        advance(p);                      /* consume '&&' */
        Node *right = parse_eq(p);
        Node *n  = node_new(AST_BINARY, line);
        n->op    = 'A';                  /* 'A' encodes && */
        n->left  = left;
        n->right = right;
        left = n;
    }
    return left;
}

/* parse_eq: equality comparison (== and !=).
 * eq_expr = rel_expr (('=='|'!=') rel_expr)* */
static Node *parse_eq(Parser *p) {
    Node *left = parse_rel(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ)) {
        int  line = p->current.line;
        char op   = check(p, TOK_EQ) ? 'E' : 'N'; /* 'E'==  'N'!= */
        advance(p);
        Node *right = parse_rel(p);
        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left = n;
    }
    return left;
}

/* parse_rel: relational comparison (<, >, <=, >=).
 * rel_expr = add_expr (('<'|'>'|'<='|'>=') add_expr)* */
static Node *parse_rel(Parser *p) {
    Node *left = parse_add(p);
    while (check(p, TOK_LT) || check(p, TOK_GT) ||
           check(p, TOK_LEQ) || check(p, TOK_GEQ)) {
        int  line = p->current.line;
        char op;
        if      (check(p, TOK_LT))  op = '<';   /* less than */
        else if (check(p, TOK_GT))  op = '>';   /* greater than */
        else if (check(p, TOK_LEQ)) op = 'L';   /* 'L' encodes <= */
        else                         op = 'G';   /* 'G' encodes >= */
        advance(p);
        Node *right = parse_add(p);
        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left = n;
    }
    return left;
}

/* parse_add: addition and subtraction.
 * add_expr = mul_expr (('+' | '-') mul_expr)* */
static Node *parse_add(Parser *p) {
    Node *left = parse_mul(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        int  line = p->current.line;
        char op   = check(p, TOK_PLUS) ? '+' : '-';
        advance(p);
        Node *right = parse_mul(p);
        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left = n;
    }
    return left;
}

/* parse_mul: multiplication, division, modulo.
 * mul_expr = unary (('*' | '/' | '%') unary)* */
static Node *parse_mul(Parser *p) {
    Node *left = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        int  line = p->current.line;
        char op;
        if      (check(p, TOK_STAR))    op = '*';
        else if (check(p, TOK_SLASH))   op = '/';
        else                             op = '%';
        advance(p);
        Node *right = parse_unary(p);
        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left = n;
    }
    return left;
}

/* parse_unary: prefix unary operators (- and !).
 * unary = ('-' | '!') unary | primary */
static Node *parse_unary(Parser *p) {
    if (check(p, TOK_MINUS) || check(p, TOK_BANG)) {
        int  line = p->current.line;
        char op   = check(p, TOK_MINUS) ? '-' : '!';
        advance(p);                       /* consume the unary operator */
        Node *operand = parse_unary(p);   /* right-recursive for chaining */
        Node *n  = node_new(AST_UNARY, line);
        n->op    = op;
        n->left  = operand;
        return n;
    }
    return parse_primary(p);
}

/* parse_primary: literals, identifiers, function calls, parenthesised exprs.
 * primary = INT_LIT
 *         | IDENT '(' arg_list ')'
 *         | IDENT
 *         | '(' expr ')' */
static Node *parse_primary(Parser *p) {
    int line = p->current.line;

    /* Integer literal */
    if (check(p, TOK_INT_LIT)) {
        Node *n = node_new(AST_INT_LIT, line);
        n->ival = p->current.ival;    /* copy parsed integer value */
        advance(p);
        return n;
    }

    /* Identifier or function call */
    if (check(p, TOK_IDENT)) {
        Token name_tok = p->current;   /* save the name token before consuming */
        advance(p);                     /* consume the identifier */

        /* If followed by '(', this is a function call. */
        if (check(p, TOK_LPAREN)) {
            advance(p);                 /* consume '(' */
            Node *call = node_new(AST_CALL, line);
            call->sval = my_strndup(name_tok.start, name_tok.len); /* heap-copy name */

            /* Parse argument list: zero or more comma-separated expressions */
            int   cap  = 4;            /* initial capacity for args array */
            call->args = malloc((size_t)cap * sizeof(Node *));
            if (!call->args) { fprintf(stderr, "OOM\n"); exit(1); }
            call->nargs = 0;

            if (!check(p, TOK_RPAREN)) {  /* if not immediately ')' there are args */
                do {
                    if (call->nargs >= cap) {
                        cap *= 2;          /* double capacity when full */
                        call->args = realloc(call->args, (size_t)cap * sizeof(Node *));
                        if (!call->args) { fprintf(stderr, "OOM\n"); exit(1); }
                    }
                    call->args[call->nargs++] = parse_expr(p); /* parse one argument */
                } while (match(p, TOK_COMMA));   /* keep going while there are commas */
            }
            expect(p, TOK_RPAREN, "')'");  /* closing paren */
            return call;
        }

        /* Plain identifier reference */
        Node *n = node_new(AST_IDENT, line);
        n->sval = my_strndup(name_tok.start, name_tok.len); /* heap-copy the name */
        return n;
    }

    /* Parenthesised sub-expression */
    if (check(p, TOK_LPAREN)) {
        advance(p);                    /* consume '(' */
        Node *inner = parse_expr(p);   /* parse the inner expression */
        expect(p, TOK_RPAREN, "')'");  /* consume ')' */
        return inner;
    }

    /* Nothing matched — unexpected token */
    fprintf(stderr, "parse error at line %d: unexpected token '%.*s' (%s) in expression\n",
            line, (int)p->current.len, p->current.start,
            token_type_name(p->current.type));
    p->had_error = 1;
    /* Return a dummy INT node to allow parsing to continue. */
    Node *dummy = node_new(AST_INT_LIT, line);
    dummy->ival = 0;
    advance(p);   /* skip the bad token so we don't loop forever */
    return dummy;
}

/* =========================================================================
 * Statement parsing  (Modules 04/05 — unchanged here)
 * ========================================================================= */

/* parse_stmt: dispatch to the correct statement handler.
 *
 * Uses two-token lookahead to distinguish:
 *   'int' IDENT        → variable declaration
 *   'if'               → if statement
 *   'while'            → while loop
 *   'return'           → return statement
 *   IDENT '='          → assignment  (current=IDENT, next='=')
 *   anything else      → expression statement
 */
Node *parse_stmt(Parser *p) {
    int line = p->current.line;

    /* Variable declaration: 'int' IDENT ... */
    if (check(p, TOK_KW_INT) && p->next.type == TOK_IDENT) {
        advance(p);                    /* consume 'int' */
        Token name_tok = p->current;
        advance(p);                    /* consume the variable name */

        Node *decl = node_new(AST_VAR_DECL, line);
        decl->sval = my_strndup(name_tok.start, name_tok.len);

        if (match(p, TOK_ASSIGN)) {    /* optional initialiser: '=' expr */
            decl->left = parse_expr(p);
        }
        expect(p, TOK_SEMICOLON, "';'");
        return decl;
    }

    /* if statement */
    if (check(p, TOK_KW_IF)) {
        advance(p);                    /* consume 'if' */
        expect(p, TOK_LPAREN, "'('");
        Node *cond = parse_expr(p);    /* condition */
        expect(p, TOK_RPAREN, "')'");
        Node *then_b = parse_block(p); /* then-branch block */

        Node *n   = node_new(AST_IF, line);
        n->left   = cond;
        n->right  = then_b;
        n->extra  = NULL;

        if (match(p, TOK_KW_ELSE)) {  /* optional else branch */
            n->extra = parse_block(p);
        }
        return n;
    }

    /* while loop */
    if (check(p, TOK_KW_WHILE)) {
        advance(p);                    /* consume 'while' */
        expect(p, TOK_LPAREN, "'('");
        Node *cond = parse_expr(p);
        expect(p, TOK_RPAREN, "')'");
        Node *body = parse_block(p);

        Node *n  = node_new(AST_WHILE, line);
        n->left  = cond;
        n->right = body;
        return n;
    }

    /* return statement */
    if (check(p, TOK_KW_RETURN)) {
        advance(p);                    /* consume 'return' */
        Node *n = node_new(AST_RETURN, line);
        if (!check(p, TOK_SEMICOLON)) {
            n->left = parse_expr(p);  /* optional return value */
        }
        expect(p, TOK_SEMICOLON, "';'");
        return n;
    }

    /* Assignment: IDENT '=' expr ';'
     * Detected with two-token lookahead: current=IDENT, next='=' */
    if (check(p, TOK_IDENT) && p->next.type == TOK_ASSIGN) {
        Token name_tok = p->current;
        advance(p);                    /* consume IDENT */
        advance(p);                    /* consume '=' */
        Node *val = parse_expr(p);
        expect(p, TOK_SEMICOLON, "';'");

        Node *n = node_new(AST_ASSIGN, line);
        n->sval = my_strndup(name_tok.start, name_tok.len);
        n->left = val;
        return n;
    }

    /* Expression statement: expr ';' */
    {
        Node *expr = parse_expr(p);
        expect(p, TOK_SEMICOLON, "';'");
        Node *n = node_new(AST_EXPR_STMT, line);
        n->left = expr;
        return n;
    }
}

/* parse_block: parse a brace-enclosed list of statements.
 * block = '{' stmt* '}'
 * Returns an AST_BLOCK node whose args[] holds the statements. */
Node *parse_block(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_LBRACE, "'{'");      /* consume the opening brace */

    Node *block = node_new(AST_BLOCK, line);
    int   cap   = 8;                   /* initial capacity */
    block->args = malloc((size_t)cap * sizeof(Node *));
    if (!block->args) { fprintf(stderr, "OOM\n"); exit(1); }
    block->nargs = 0;

    /* parse statements until we see '}' or EOF */
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (block->nargs >= cap) {
            cap *= 2;                  /* grow the array */
            block->args = realloc(block->args, (size_t)cap * sizeof(Node *));
            if (!block->args) { fprintf(stderr, "OOM\n"); exit(1); }
        }
        block->args[block->nargs++] = parse_stmt(p);  /* parse one statement */
    }
    expect(p, TOK_RBRACE, "'}'");      /* consume the closing brace */
    return block;
}

/* =========================================================================
 * Function and program parsing  (Module 06 — new implementations)
 * ========================================================================= */

/* parse_function: parse a single top-level function definition.
 *
 * Grammar:
 *   function   = ('int' | 'void') IDENT '(' param_list ')' block
 *   param_list = ( type IDENT (',' type IDENT)* )?
 *   type       = 'int' | 'void'
 *
 * Returns an AST_FUNC node with:
 *   sval   = heap-copied function name
 *   op     = 'i' for int return type, 'v' for void return type
 *   args[] = one AST_VAR_DECL per parameter (no initialiser)
 *   nargs  = number of parameters
 *   right  = the AST_BLOCK for the function body
 */
Node *parse_function(Parser *p) {
    int line = p->current.line;

    /* --- Return type --- */
    char ret_type;
    if (check(p, TOK_KW_INT)) {
        ret_type = 'i';                /* 'i' = int */
        advance(p);                    /* consume 'int' */
    } else if (check(p, TOK_KW_VOID)) {
        ret_type = 'v';                /* 'v' = void */
        advance(p);                    /* consume 'void' */
    } else {
        fprintf(stderr, "parse error at line %d: expected 'int' or 'void'\n", line);
        p->had_error = 1;
        ret_type = 'i';               /* recover: assume int */
    }

    /* --- Function name --- */
    Token name_tok = p->current;
    expect(p, TOK_IDENT, "function name");   /* consume the identifier */

    /* Build the AST_FUNC node */
    Node *func = node_new(AST_FUNC, line);
    func->sval = my_strndup(name_tok.start, name_tok.len); /* heap-copy name */
    func->op   = ret_type;             /* store return-type as a char in op */

    /* --- Parameter list --- */
    expect(p, TOK_LPAREN, "'('");      /* consume opening paren */

    int   cap   = 4;                   /* initial capacity for params array */
    func->args  = malloc((size_t)cap * sizeof(Node *));
    if (!func->args) { fprintf(stderr, "OOM\n"); exit(1); }
    func->nargs = 0;

    /* A lone 'void' inside '(' ... ')' means zero parameters — C99 style. */
    if (check(p, TOK_KW_VOID) && p->next.type == TOK_RPAREN) {
        advance(p);   /* consume 'void' — param count stays 0 */
    } else if (!check(p, TOK_RPAREN)) {
        /* Parse one or more typed parameters. */
        do {
            /* Each parameter is: type IDENT */
            int param_line = p->current.line;

            /* Consume the parameter type ('int' or 'void'). */
            if (!check(p, TOK_KW_INT) && !check(p, TOK_KW_VOID)) {
                fprintf(stderr,
                        "parse error at line %d: expected parameter type\n",
                        param_line);
                p->had_error = 1;
            } else {
                advance(p);            /* consume 'int' or 'void' */
            }

            /* Parameter name */
            Token pname = p->current;
            expect(p, TOK_IDENT, "parameter name");

            /* Build an AST_VAR_DECL for the parameter (no initialiser). */
            Node *param = node_new(AST_VAR_DECL, param_line);
            param->sval = my_strndup(pname.start, pname.len);
            param->left = NULL;        /* no initialiser for parameters */

            /* Grow the args array if needed. */
            if (func->nargs >= cap) {
                cap *= 2;
                func->args = realloc(func->args, (size_t)cap * sizeof(Node *));
                if (!func->args) { fprintf(stderr, "OOM\n"); exit(1); }
            }
            func->args[func->nargs++] = param; /* append this parameter */

        } while (match(p, TOK_COMMA));   /* keep parsing if there's a comma */
    }

    expect(p, TOK_RPAREN, "')'");     /* consume closing paren */

    /* --- Function body --- */
    func->right = parse_block(p);     /* parse the brace-enclosed body */

    return func;
}

/* parse_program: parse a whole source file as a sequence of functions.
 *
 * Grammar: program = function*
 *
 * Loop: while the current token is 'int' or 'void', parse a function.
 * A stray token at the top level stops the loop (no error emitted here —
 * in production you'd emit one; see DECISIONS.md for rationale).
 *
 * Returns an AST_PROGRAM node whose args[] holds each AST_FUNC.
 */
Node *parse_program(Parser *p) {
    Node *prog = node_new(AST_PROGRAM, p->current.line);

    int   cap   = 8;                   /* initial capacity for function array */
    prog->args  = malloc((size_t)cap * sizeof(Node *));
    if (!prog->args) { fprintf(stderr, "OOM\n"); exit(1); }
    prog->nargs = 0;

    /* Keep parsing functions as long as we see a type keyword at top level. */
    while (check(p, TOK_KW_INT) || check(p, TOK_KW_VOID)) {
        if (prog->nargs >= cap) {
            cap *= 2;                  /* double capacity when full */
            prog->args = realloc(prog->args, (size_t)cap * sizeof(Node *));
            if (!prog->args) { fprintf(stderr, "OOM\n"); exit(1); }
        }
        prog->args[prog->nargs++] = parse_function(p); /* parse one function */
    }

    /* Warn if we stopped before EOF — indicates unexpected top-level content. */
    if (!check(p, TOK_EOF)) {
        fprintf(stderr,
                "parse warning at line %d: unexpected token at top level: '%.*s'\n",
                p->current.line, (int)p->current.len, p->current.start);
    }

    return prog;
}
