/* parser.c — Recursive descent expression parser for my_compiler
 * Module 04 — expression parsing introduced here.
 * Prerequisites: parser.h, ast.h, lexer.h, token.h, source.h.
 *
 * Grammar handled in this module (expressions only):
 *
 *   expr           = logical
 *   logical        = comparison (('&&'|'||') comparison)*
 *   comparison     = additive (('=='|'!='|'<'|'>'|'<='|'>=') additive)*
 *   additive       = multiplicative (('+'|'-') multiplicative)*
 *   multiplicative = unary (('*'|'/'|'%') unary)*
 *   unary          = '-' unary | primary
 *   primary        = INT_LIT
 *                  | IDENT '(' arglist ')'
 *                  | IDENT
 *                  | '(' expr ')'
 *   arglist        = (expr (',' expr)*)?
 *
 * Statements (parse_stmt, parse_block) are stubs — added in Module 05.
 * Functions and programs (parse_function, parse_program) are stubs — Module 06.
 */
#include "parser.h"

#include <stdio.h>   /* fprintf, printf */
#include <stdlib.h>  /* malloc, realloc, free, exit */
#include <string.h>  /* memcpy */

/* =========================================================================
 * Internal helper: my_strndup
 * Portable replacement for POSIX strndup (not in C11 standard library).
 * ====================================================================== */

/* my_strndup: allocate a copy of the first 'n' bytes of 's', NUL-terminated.
 * The caller is responsible for free()ing the returned pointer. */
static char *my_strndup(const char *s, size_t n) {
    char *p = malloc(n + 1);             /* +1 for NUL terminator */
    if (!p) {
        fprintf(stderr, "out of memory in my_strndup\n");
        exit(1);
    }
    memcpy(p, s, n);                     /* copy exactly n bytes */
    p[n] = '\0';                         /* NUL-terminate */
    return p;
}

/* =========================================================================
 * Lexer-level helpers: advance, check, match, expect, error
 * ====================================================================== */

/* advance: consume the current token.
 * The consumed token is saved in p->prev; the next token becomes p->current.
 *
 * Params:  p — the parser
 * Returns: nothing (modifies p in place)
 * Assumes: p has been initialised with parser_init
 */
static void advance(Parser *p) {
    p->prev    = p->current;             /* save the token we are about to consume */
    p->current = lexer_next(&p->lex);   /* fetch the next token from the lexer */
}

/* check: return 1 if the current token has the given type, 0 otherwise.
 *
 * Params:  p    — the parser
 *          type — the token type to test
 * Returns: 1 if p->current.type == type, else 0
 * Assumes: parser_init has been called
 */
static int check(const Parser *p, TokenType type) {
    return p->current.type == type;      /* simple equality test */
}

/* match: if current token matches 'type', consume it and return 1.
 *        Otherwise return 0 without consuming anything.
 *
 * Params:  p    — the parser
 *          type — the token type to try to match
 * Returns: 1 if matched and consumed, 0 if not
 */
static int match(Parser *p, TokenType type) {
    if (!check(p, type)) return 0;      /* not a match — do nothing */
    advance(p);                          /* consume the matching token */
    return 1;
}

/* expect: assert that the current token is 'type'; if so, consume it.
 *         If not, print an error message and set had_error=1.
 *
 * Params:  p    — the parser
 *          type — the token type we require
 *          msg  — human-readable description used in the error message
 * Returns: 1 on success, 0 on failure
 */
static int expect(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) {
        advance(p);                      /* happy path: consume the expected token */
        return 1;
    }
    /* Error path: report what we found vs what we needed */
    fprintf(stderr, "error at line %d: %s (got %s)\n",
            p->current.line, msg, token_type_name(p->current.type));
    p->had_error = 1;
    return 0;
}

/* error: report a parse error at the current position.
 *
 * Params:  p   — the parser
 *          msg — human-readable description of the problem
 */
static void error(Parser *p, const char *msg) {
    fprintf(stderr, "error at line %d: %s\n", p->current.line, msg);
    p->had_error = 1;
}

/* =========================================================================
 * Forward declarations (recursive grammar requires forward refs)
 * ====================================================================== */
static Node *parse_logical(Parser *p);
static Node *parse_comparison(Parser *p);
static Node *parse_additive(Parser *p);
static Node *parse_multiplicative(Parser *p);
static Node *parse_unary(Parser *p);
static Node *parse_primary(Parser *p);

/* =========================================================================
 * parser_init
 * ====================================================================== */

/* parser_init: set up *p to parse the given Source.
 *
 * Params:  p   — uninitialised Parser to fill in
 *          src — the Source whose tokens we will parse
 * Returns: nothing
 * Assumes: src has been opened (source_open or source_from_string)
 */
void parser_init(Parser *p, const Source *src) {
    lexer_init(&p->lex, src);           /* point the lexer at the source */
    p->had_error = 0;                   /* no errors yet */
    /* Prime the lookahead: load the very first token into p->current */
    advance(p);
}

/* =========================================================================
 * Expression grammar — one function per precedence level
 *
 * Each function handles operators at ONE precedence level and delegates
 * higher-precedence work to the function below it.  The result is that
 * higher-precedence operators bind more tightly — exactly what we want.
 * ====================================================================== */

/* parse_expr: top-level expression entry point.
 *
 * Params:  p — the parser
 * Returns: root Node of the expression tree
 *
 * This is simply an alias for parse_logical; it exists so that callers
 * always call parse_expr rather than knowing about the internal hierarchy.
 */
Node *parse_expr(Parser *p) {
    return parse_logical(p);             /* delegate to the lowest-precedence layer */
}

/* parse_logical: handle && and || operators (lowest precedence in expressions).
 *
 * Grammar: logical = comparison (('&&' | '||') comparison)*
 *
 * Left-associativity is achieved with a while loop: we parse a left operand,
 * then repeatedly check for a logical operator and parse the right operand,
 * building a left-leaning binary tree.
 *
 * Operator mapping:  && → op='A'   || → op='O'
 *
 * Params:  p — the parser
 * Returns: expression Node
 */
static Node *parse_logical(Parser *p) {
    Node *left = parse_comparison(p);   /* parse the left-hand comparison */

    /* Keep consuming logical operators as long as we see them */
    while (check(p, TOK_AMPAMP) || check(p, TOK_PIPEPIPE)) {
        char   op   = check(p, TOK_AMPAMP) ? 'A' : 'O'; /* encode operator */
        int    line = p->current.line;   /* record line before consuming */
        advance(p);                      /* consume && or || */

        Node *right = parse_comparison(p); /* parse the right-hand comparison */

        /* Build a BINARY node with left and right as children */
        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left     = n;                    /* the new node becomes the new left */
    }
    return left;
}

/* parse_comparison: handle ==, !=, <, >, <=, >= (second-lowest precedence).
 *
 * Grammar: comparison = additive (('=='|'!='|'<'|'>'|'<='|'>=') additive)*
 *
 * Operator mapping:  == → 'E'   != → 'N'   <= → 'L'   >= → 'G'
 *                    <  → '<'   >  → '>'
 *
 * Params:  p — the parser
 * Returns: expression Node
 */
static Node *parse_comparison(Parser *p) {
    Node *left = parse_additive(p);     /* left operand */

    /* Try each comparison operator in turn */
    while (check(p, TOK_EQ)  || check(p, TOK_NEQ) ||
           check(p, TOK_LT)  || check(p, TOK_GT)  ||
           check(p, TOK_LEQ) || check(p, TOK_GEQ)) {

        /* Encode the operator as a single char */
        char op;
        switch (p->current.type) {
        case TOK_EQ:  op = 'E'; break;  /* == */
        case TOK_NEQ: op = 'N'; break;  /* != */
        case TOK_LEQ: op = 'L'; break;  /* <= */
        case TOK_GEQ: op = 'G'; break;  /* >= */
        case TOK_LT:  op = '<'; break;  /* <  */
        case TOK_GT:  op = '>'; break;  /* >  */
        default:      op = '?'; break;  /* unreachable */
        }
        int line = p->current.line;
        advance(p);                      /* consume the comparison operator */

        Node *right = parse_additive(p); /* right operand */

        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left     = n;
    }
    return left;
}

/* parse_additive: handle + and - (medium precedence).
 *
 * Grammar: additive = multiplicative (('+' | '-') multiplicative)*
 *
 * WHY a while loop instead of direct recursion:
 *   The "natural" left-recursive rule  additive = additive '+' multiplicative
 *   would cause parse_additive to immediately call itself — infinite recursion.
 *   The while loop is the standard fix: parse the first multiplicative, then
 *   loop consuming operator+multiplicative pairs, building a left tree.
 *
 * Params:  p — the parser
 * Returns: expression Node
 */
static Node *parse_additive(Parser *p) {
    Node *left = parse_multiplicative(p); /* left operand (higher precedence) */

    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        char op   = check(p, TOK_PLUS) ? '+' : '-'; /* which operator */
        int  line = p->current.line;
        advance(p);                      /* consume + or - */

        Node *right = parse_multiplicative(p); /* right operand */

        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left     = n;                    /* left-fold: new node becomes left */
    }
    return left;
}

/* parse_multiplicative: handle *, /, % (high precedence).
 *
 * Grammar: multiplicative = unary (('*' | '/' | '%') unary)*
 *
 * Same left-associative while-loop pattern as parse_additive.
 *
 * Params:  p — the parser
 * Returns: expression Node
 */
static Node *parse_multiplicative(Parser *p) {
    Node *left = parse_unary(p);        /* left operand */

    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        char op;
        switch (p->current.type) {
        case TOK_STAR:    op = '*'; break;
        case TOK_SLASH:   op = '/'; break;
        case TOK_PERCENT: op = '%'; break;
        default:          op = '?'; break; /* unreachable */
        }
        int line = p->current.line;
        advance(p);                      /* consume *, / or % */

        Node *right = parse_unary(p);   /* right operand */

        Node *n  = node_new(AST_BINARY, line);
        n->op    = op;
        n->left  = left;
        n->right = right;
        left     = n;
    }
    return left;
}

/* parse_unary: handle prefix unary minus (highest-precedence unary).
 *
 * Grammar: unary = '-' unary | primary
 *
 * Right-recursive: -(-(x)) is valid and parses as UNARY('-', UNARY('-', x)).
 * Only unary minus is supported here; unary ! would be added similarly.
 *
 * Params:  p — the parser
 * Returns: expression Node
 */
static Node *parse_unary(Parser *p) {
    if (match(p, TOK_MINUS)) {           /* found a leading '-' */
        int   line = p->prev.line;       /* line of the '-' operator */
        Node *operand = parse_unary(p);  /* recurse for right-associativity */
        Node *n  = node_new(AST_UNARY, line);
        n->op    = '-';
        n->left  = operand;
        return n;
    }
    return parse_primary(p);             /* no unary op — fall through to primary */
}

/* parse_primary: handle atoms and function calls (highest precedence).
 *
 * Grammar:
 *   primary = INT_LIT
 *           | IDENT '(' arglist ')'
 *           | IDENT
 *           | '(' expr ')'
 *   arglist = (expr (',' expr)*)?
 *
 * The IDENT vs IDENT '(' decision requires one token of lookahead:
 * after consuming the IDENT, check whether the NEXT token is '('.
 *
 * Params:  p — the parser
 * Returns: expression Node, or NULL on error
 */
static Node *parse_primary(Parser *p) {

    /* ----- Integer literal ----- */
    if (match(p, TOK_INT_LIT)) {
        Node *n = node_new(AST_INT_LIT, p->prev.line);
        n->ival = p->prev.ival;          /* value was parsed by the lexer */
        return n;
    }

    /* ----- Identifier or function call ----- */
    if (match(p, TOK_IDENT)) {
        Token ident_tok = p->prev;       /* save the identifier token */

        if (match(p, TOK_LPAREN)) {
            /* ------ Function call: IDENT '(' arglist ')' ------ */
            Node *n  = node_new(AST_CALL, ident_tok.line);
            n->sval  = my_strndup(ident_tok.start, ident_tok.len); /* copy name */
            n->args  = NULL;
            n->nargs = 0;

            /* Parse argument list (may be empty) */
            if (!check(p, TOK_RPAREN)) {
                /* At least one argument */
                do {
                    Node *arg = parse_expr(p);              /* parse one argument */
                    /* Grow the args array with realloc */
                    n->args = realloc(n->args, (size_t)(n->nargs + 1) * sizeof(Node*));
                    if (!n->args) {
                        fprintf(stderr, "out of memory in parse_primary (call args)\n");
                        exit(1);
                    }
                    n->args[n->nargs++] = arg;              /* append the argument */
                } while (match(p, TOK_COMMA));              /* keep going while there are commas */
            }

            expect(p, TOK_RPAREN, "expected ')' after argument list");
            return n;

        } else {
            /* ------ Simple identifier reference ------ */
            Node *n = node_new(AST_IDENT, ident_tok.line);
            n->sval = my_strndup(ident_tok.start, ident_tok.len); /* copy name */
            return n;
        }
    }

    /* ----- Parenthesised expression: '(' expr ')' ----- */
    if (match(p, TOK_LPAREN)) {
        Node *inner = parse_expr(p);     /* parse the inner expression */
        expect(p, TOK_RPAREN, "expected ')' after expression");
        return inner;                    /* the parens themselves don't create a node */
    }

    /* ----- Nothing matched: report an error ----- */
    error(p, "expected an expression");
    return NULL;
}

/* =========================================================================
 * Stub implementations — will be filled in later modules
 * ====================================================================== */

/* parse_program: parse a complete program.
 * Implemented in Module 06.  Currently a stub returning NULL.
 *
 * Params:  p — the parser (unused in stub)
 * Returns: NULL
 */
Node *parse_program(Parser *p) {
    (void)p;   /* suppress unused-parameter warning */
    return NULL;
}

/* parse_function: parse one function definition.
 * Implemented in Module 06.  Currently a stub returning NULL.
 *
 * Params:  p — the parser (unused in stub)
 * Returns: NULL
 */
Node *parse_function(Parser *p) {
    (void)p;
    return NULL;
}

/* parse_block: parse a braced block of statements.
 * Implemented in Module 05.  Currently a stub returning NULL.
 *
 * Params:  p — the parser (unused in stub)
 * Returns: NULL
 */
Node *parse_block(Parser *p) {
    (void)p;
    return NULL;
}

/* parse_stmt: parse a single statement.
 * Implemented in Module 05.  Currently a stub returning NULL.
 *
 * Params:  p — the parser (unused in stub)
 * Returns: NULL
 */
Node *parse_stmt(Parser *p) {
    (void)p;
    return NULL;
}
