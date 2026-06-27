/* parser.c -- Recursive descent parser for my_compiler
 * Module 05 -- full statement + program parsing.
 * Prerequisites: parser.h, ast.h, lexer.h, token.h, source.h.
 *
 * Grammar handled in this module:
 *
 *   program        = func*
 *   func           = "int" IDENT "(" params ")" block
 *   params         = ("void" | (param ("," param)*))?
 *   param          = "int" IDENT
 *   block          = "{" stmt* "}"
 *   stmt           = block
 *                  | "if" "(" expr ")" stmt ("else" stmt)?
 *                  | "while" "(" expr ")" stmt
 *                  | "return" expr ";"
 *                  | "int" IDENT ("=" expr)? ";"
 *                  | expr ";"
 *   expr           = IDENT "=" expr          (right-assoc assignment)
 *                  | logical
 *   logical        = comparison (("&&"|"||") comparison)*
 *   comparison     = additive  (("=="|"!="|"<"|">"|"<="|">=") additive)*
 *   additive       = multiplicative (("+"| "-") multiplicative)*
 *   multiplicative = unary (("*"|"/"|"%") unary)*
 *   unary          = "-" unary | "!" unary | primary
 *   primary        = INT_LIT
 *                  | IDENT "(" arglist ")"
 *                  | IDENT
 *                  | "(" expr ")"
 *   arglist        = (expr ("," expr)*)?
 */
#include "parser.h"

#include <stdio.h>   /* fprintf, printf */
#include <stdlib.h>  /* malloc, realloc, free, exit */
#include <string.h>  /* memcpy */

/* =========================================================================
 * my_strndup -- portable strndup (not in C11 standard library)
 * ====================================================================== */

/* my_strndup: heap-copy the first 'n' bytes of 's' and NUL-terminate.
 * WHY we need this: Token.start points into the Source buffer; we must
 * copy the name into its own allocation so the AST node owns its string
 * independently of the Source lifetime. */
static char *my_strndup(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (!p) {
        fprintf(stderr, "out of memory in my_strndup\n");
        exit(1);
    }
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

/* =========================================================================
 * Token navigation helpers
 * ====================================================================== */

/* advance: consume cur, shift peek into cur, fetch a new peek.
 *
 * WHY two tokens in the buffer?
 *   Some grammar decisions require seeing TWO tokens ahead.  Example:
 *   "int" alone could begin a variable declaration or a function definition.
 *   With peek we can see the identifier that follows "int" without consuming
 *   it, keeping backtracking unnecessary.
 */
static void advance(Parser *p) {
    p->cur  = p->peek;                   /* move the lookahead forward */
    p->peek = lexer_next(&p->lex);       /* fetch the next token */
}

/* check: true if cur has the given type. */
static int check(const Parser *p, TokenType t) {
    return p->cur.type == t;
}

/* match: consume cur if it has the given type; return 1 on success. */
static int match(Parser *p, TokenType t) {
    if (!check(p, t)) return 0;
    advance(p);
    return 1;
}

/* expect: consume cur if it matches 'type'; otherwise print an error and exit.
 *
 * WHY exit() instead of error recovery?
 *   For a teaching compiler, stopping immediately gives the clearest message.
 *   Real compilers implement panic-mode recovery to surface multiple errors
 *   in a single run, but that adds a lot of complexity (see DECISIONS.md).
 */
static void expect(Parser *p, TokenType type, const char *what) {
    if (check(p, type)) {
        advance(p);
        return;
    }
    fprintf(stderr, "parse error: expected %s got %s at line %d\n",
            what, token_type_name(p->cur.type), p->cur.line);
    exit(1);
}

/* parse_error: print an error message and exit unconditionally.
 * Use when no single "expected" token can describe the problem. */
static void parse_error(Parser *p, const char *msg) {
    fprintf(stderr, "parse error: %s got %s at line %d\n",
            msg, token_type_name(p->cur.type), p->cur.line);
    exit(1);
}

/* =========================================================================
 * Forward declarations (recursive grammar needs them)
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

/* parser_init: prepare the parser by priming both lookahead slots.
 *
 * After this call:
 *   p->cur  == the first real token in the source
 *   p->peek == the second real token in the source
 *
 * We call lexer_next twice because the lexer starts with no token fetched.
 */
void parser_init(Parser *p, Lexer *lex) {
    p->lex  = *lex;                      /* copy the lexer (owns scan state) */
    /* Prime the two-slot lookahead by fetching two tokens up front */
    p->cur  = lexer_next(&p->lex);
    p->peek = lexer_next(&p->lex);
}

/* =========================================================================
 * parse_program
 * ====================================================================== */

/* parse_program: parse a sequence of function definitions until EOF.
 *
 * Returns an AST_PROGRAM node; its args[] array holds each AST_FUNC in order.
 * A real C program can have multiple functions, so we keep parsing until
 * we run out of tokens.
 */
Node *parse_program(Parser *p) {
    Node *prog   = node_new(AST_PROGRAM, p->cur.line);
    prog->args   = NULL;
    prog->nargs  = 0;

    /* Keep parsing functions until we hit end-of-file */
    while (!check(p, TOK_EOF)) {
        Node *fn = parse_func(p);
        /* Grow the function list with realloc -- we do not know the count
         * in advance so we expand one slot at a time. */
        prog->args = realloc(prog->args,
                             (size_t)(prog->nargs + 1) * sizeof(Node *));
        if (!prog->args) {
            fprintf(stderr, "out of memory in parse_program\n");
            exit(1);
        }
        prog->args[prog->nargs++] = fn;
    }
    return prog;
}

/* =========================================================================
 * parse_func
 * ====================================================================== */

/* parse_func: parse one complete function definition.
 *
 * Accepted grammar:
 *   func = "int" IDENT "(" params ")" block
 *   params = "void"
 *          | (param ("," param)*)
 *          | (empty)
 *   param = "int" IDENT
 *
 * WHY we require the return type to be "int":
 *   Our tiny language only supports int-returning functions at this stage.
 *   "void" return types are not parsed here -- that would be added in a
 *   later module once we have a type system.
 *
 * HOW void parameter lists are handled:
 *   C allows "foo(void)" to mean "no parameters".  We detect the single
 *   token "void" inside the parentheses and skip it, producing an AST_FUNC
 *   node with nargs==0.  This matches the C standard convention.
 */
Node *parse_func(Parser *p) {
    int line = p->cur.line;

    /* Consume the "int" return type keyword */
    expect(p, TOK_KW_INT, "'int'");

    /* Consume the function name */
    if (!check(p, TOK_IDENT)) {
        parse_error(p, "expected function name");
    }
    char *fname = my_strndup(p->cur.start, p->cur.len);
    int   fline = p->cur.line;
    advance(p);  /* consume the identifier */

    /* Opening parenthesis of the parameter list */
    expect(p, TOK_LPAREN, "'('");

    Node *func   = node_new(AST_FUNC, fline);
    func->sval   = fname;
    func->args   = NULL;
    func->nargs  = 0;
    (void)line;  /* line variable used only for debugging; fline covers it */

    /* Parse parameter list */
    if (check(p, TOK_KW_VOID)) {
        /* "void" means no parameters -- consume and move on */
        advance(p);
    } else if (!check(p, TOK_RPAREN)) {
        /* One or more "int IDENT" parameters */
        do {
            expect(p, TOK_KW_INT, "'int' before parameter name");
            if (!check(p, TOK_IDENT)) {
                parse_error(p, "expected parameter name");
            }
            /* Build an AST_IDENT node to represent each parameter.
             * We reuse AST_IDENT here because a parameter is just a named
             * slot -- the type is always "int" at this stage. */
            Node *param = node_new(AST_IDENT, p->cur.line);
            param->sval = my_strndup(p->cur.start, p->cur.len);
            advance(p);  /* consume parameter name */

            func->args = realloc(func->args,
                                 (size_t)(func->nargs + 1) * sizeof(Node *));
            if (!func->args) {
                fprintf(stderr, "out of memory in parse_func (params)\n");
                exit(1);
            }
            func->args[func->nargs++] = param;
        } while (match(p, TOK_COMMA));
    }

    expect(p, TOK_RPAREN, "')'");

    /* Parse the function body as a block */
    func->right = parse_block(p);
    return func;
}

/* =========================================================================
 * parse_block
 * ====================================================================== */

/* parse_block: parse a braced list of statements.
 *
 * Grammar: block = "{" stmt* "}"
 *
 * Returns an AST_BLOCK node; stmt nodes live in args[].
 *
 * WHY AST_BLOCK uses args[] instead of a linked list:
 *   A dynamically-grown array (via realloc) gives O(1) indexed access later
 *   during code generation.  A linked list would require traversal.  The
 *   trade-off is slightly more complex allocation, but it pays off in every
 *   later pass.  See DECISIONS.md for more.
 */
Node *parse_block(Parser *p) {
    int line = p->cur.line;
    expect(p, TOK_LBRACE, "'{'");

    Node *block  = node_new(AST_BLOCK, line);
    block->args  = NULL;
    block->nargs = 0;

    /* Parse statements until we see the closing brace */
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Node *stmt = parse_stmt(p);
        block->args = realloc(block->args,
                              (size_t)(block->nargs + 1) * sizeof(Node *));
        if (!block->args) {
            fprintf(stderr, "out of memory in parse_block\n");
            exit(1);
        }
        block->args[block->nargs++] = stmt;
    }

    expect(p, TOK_RBRACE, "'}'");
    return block;
}

/* =========================================================================
 * parse_stmt
 * ====================================================================== */

/* parse_stmt: parse a single statement.
 *
 * The decision tree:
 *   '{'       -> compound statement (block)
 *   "if"      -> if statement (with optional else)
 *   "while"   -> while loop
 *   "return"  -> return statement
 *   "int"     -> variable declaration
 *   anything  -> expression statement
 *
 * This is a classic "dispatcher" -- we look at the current token to decide
 * which parse function to call.  No backtracking needed because each keyword
 * uniquely identifies the statement kind.
 */
Node *parse_stmt(Parser *p) {

    /* ---- Compound statement ---- */
    if (check(p, TOK_LBRACE)) {
        return parse_block(p);
    }

    /* ---- if statement ---- */
    if (check(p, TOK_KW_IF)) {
        int line = p->cur.line;
        advance(p);  /* consume "if" */

        expect(p, TOK_LPAREN, "'(' after 'if'");
        Node *cond = parse_expr(p);
        expect(p, TOK_RPAREN, "')' after if condition");

        Node *then_branch = parse_stmt(p);

        /* Optional else clause: peek to see if "else" follows */
        Node *else_branch = NULL;
        if (match(p, TOK_KW_ELSE)) {
            else_branch = parse_stmt(p);
        }

        Node *n  = node_new(AST_IF, line);
        n->left  = cond;
        n->right = then_branch;
        n->extra = else_branch;  /* NULL if no else */
        return n;
    }

    /* ---- while loop ---- */
    if (check(p, TOK_KW_WHILE)) {
        int line = p->cur.line;
        advance(p);  /* consume "while" */

        expect(p, TOK_LPAREN, "'(' after 'while'");
        Node *cond = parse_expr(p);
        expect(p, TOK_RPAREN, "')' after while condition");

        Node *body = parse_stmt(p);

        Node *n  = node_new(AST_WHILE, line);
        n->left  = cond;
        n->right = body;
        return n;
    }

    /* ---- return statement ---- */
    if (check(p, TOK_KW_RETURN)) {
        int line = p->cur.line;
        advance(p);  /* consume "return" */

        Node *n   = node_new(AST_RETURN, line);
        n->left   = parse_expr(p);   /* the value to return */
        expect(p, TOK_SEMICOLON, "';' after return expression");
        return n;
    }

    /* ---- variable declaration: "int" IDENT ("=" expr)? ";" ---- */
    if (check(p, TOK_KW_INT)) {
        int line = p->cur.line;
        advance(p);  /* consume "int" */

        if (!check(p, TOK_IDENT)) {
            parse_error(p, "expected variable name after 'int'");
        }
        char *name = my_strndup(p->cur.start, p->cur.len);
        advance(p);  /* consume the identifier */

        Node *decl = node_new(AST_VAR_DECL, line);
        decl->sval = name;

        /* Optional initialiser: "= expr" */
        if (match(p, TOK_ASSIGN)) {
            decl->left = parse_expr(p);
        }

        expect(p, TOK_SEMICOLON, "';' after variable declaration");
        return decl;
    }

    /* ---- expression statement: expr ";" ---- */
    {
        int  line  = p->cur.line;
        Node *expr = parse_expr(p);
        expect(p, TOK_SEMICOLON, "';' after expression statement");
        Node *es   = node_new(AST_EXPR_STMT, line);
        es->left   = expr;
        return es;
    }
}

/* =========================================================================
 * parse_expr and the expression precedence hierarchy
 *
 * Precedence table (highest number = tightest binding):
 *   1. assignment      IDENT = expr      (right-associative)
 *   2. logical         &&  ||
 *   3. comparison      ==  !=  <  >  <=  >=
 *   4. additive        +  -
 *   5. multiplicative  *  /  %
 *   6. unary           -  !
 *   7. primary         literals, identifiers, calls, parenthesised exprs
 *
 * Each precedence level is one function.  Higher-precedence functions are
 * called by lower-precedence ones, creating the correct binding.
 * ====================================================================== */

/* parse_expr: top-level expression, handles assignment.
 *
 * Grammar: expr = IDENT "=" expr | logical
 *
 * WHY assignment is right-associative:
 *   "a = b = 5" should parse as "a = (b = 5)", not "(a = b) = 5".
 *   Right-recursion (calling parse_expr again for the RHS) gives us that.
 *
 * HOW we distinguish "IDENT = expr" from a plain identifier expression:
 *   We look TWO tokens ahead: if cur is IDENT and peek is '=', it is
 *   assignment.  Otherwise we fall through to parse_logical.
 *   This is exactly why the Parser struct holds both cur and peek.
 */
Node *parse_expr(Parser *p) {
    /* Assignment: cur is an identifier AND the next token is '=' */
    if (check(p, TOK_IDENT) && p->peek.type == TOK_ASSIGN) {
        int   line = p->cur.line;
        char *name = my_strndup(p->cur.start, p->cur.len);
        advance(p);  /* consume the identifier */
        advance(p);  /* consume the '=' */

        Node *rhs  = parse_expr(p);  /* right-recursive for right-associativity */

        Node *n    = node_new(AST_ASSIGN, line);
        n->sval    = name;
        n->left    = rhs;
        return n;
    }

    /* Not an assignment -- parse as a logical expression */
    return parse_logical(p);
}

/* parse_logical: handles && and || (lowest expression precedence). */
static Node *parse_logical(Parser *p) {
    Node *left = parse_comparison(p);

    while (check(p, TOK_AMPAMP) || check(p, TOK_PIPEPIPE)) {
        char op   = check(p, TOK_AMPAMP) ? 'A' : 'O';
        int  line = p->cur.line;
        advance(p);

        Node *right = parse_comparison(p);
        Node *n   = node_new(AST_BINARY, line);
        n->op     = op;
        n->left   = left;
        n->right  = right;
        left      = n;
    }
    return left;
}

/* parse_comparison: handles ==, !=, <, >, <=, >= */
static Node *parse_comparison(Parser *p) {
    Node *left = parse_additive(p);

    while (check(p, TOK_EQ)  || check(p, TOK_NEQ) ||
           check(p, TOK_LT)  || check(p, TOK_GT)  ||
           check(p, TOK_LEQ) || check(p, TOK_GEQ)) {
        char op;
        switch (p->cur.type) {
        case TOK_EQ:  op = 'E'; break;   /* == */
        case TOK_NEQ: op = 'N'; break;   /* != */
        case TOK_LEQ: op = 'L'; break;   /* <= */
        case TOK_GEQ: op = 'G'; break;   /* >= */
        case TOK_LT:  op = '<'; break;   /* <  */
        case TOK_GT:  op = '>'; break;   /* >  */
        default:      op = '?'; break;   /* unreachable */
        }
        int line = p->cur.line;
        advance(p);

        Node *right = parse_additive(p);
        Node *n   = node_new(AST_BINARY, line);
        n->op     = op;
        n->left   = left;
        n->right  = right;
        left      = n;
    }
    return left;
}

/* parse_additive: handles + and - */
static Node *parse_additive(Parser *p) {
    Node *left = parse_multiplicative(p);

    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        char op   = check(p, TOK_PLUS) ? '+' : '-';
        int  line = p->cur.line;
        advance(p);

        Node *right = parse_multiplicative(p);
        Node *n   = node_new(AST_BINARY, line);
        n->op     = op;
        n->left   = left;
        n->right  = right;
        left      = n;
    }
    return left;
}

/* parse_multiplicative: handles *, /, % */
static Node *parse_multiplicative(Parser *p) {
    Node *left = parse_unary(p);

    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        char op;
        switch (p->cur.type) {
        case TOK_STAR:    op = '*'; break;
        case TOK_SLASH:   op = '/'; break;
        case TOK_PERCENT: op = '%'; break;
        default:          op = '?'; break;
        }
        int line = p->cur.line;
        advance(p);

        Node *right = parse_unary(p);
        Node *n   = node_new(AST_BINARY, line);
        n->op     = op;
        n->left   = left;
        n->right  = right;
        left      = n;
    }
    return left;
}

/* parse_unary: handles prefix - and !
 *
 * WHY right-recursive?
 *   "- - x" is valid: the outer minus applies to (- x).  Right-recursion
 *   (calling parse_unary again) builds the correct right-leaning tree
 *   without any extra loop logic.
 */
static Node *parse_unary(Parser *p) {
    if (check(p, TOK_MINUS) || check(p, TOK_BANG)) {
        char op   = check(p, TOK_MINUS) ? '-' : '!';
        int  line = p->cur.line;
        advance(p);

        Node *operand = parse_unary(p);  /* right-recursive */
        Node *n  = node_new(AST_UNARY, line);
        n->op    = op;
        n->left  = operand;
        return n;
    }
    return parse_primary(p);
}

/* parse_primary: handles integer literals, identifiers, calls, and parens.
 *
 * Grammar:
 *   primary = INT_LIT
 *           | IDENT "(" arglist ")"
 *           | IDENT
 *           | "(" expr ")"
 *
 * The IDENT vs call distinction: we check peek for '(' AFTER consuming
 * the identifier.  At this level we have already consumed cur (the ident),
 * so peek is now the token after it.  If peek was '(' before the consume,
 * cur (after advance) is '(' -- we use a saved copy of the ident token.
 */
static Node *parse_primary(Parser *p) {

    /* Integer literal */
    if (check(p, TOK_INT_LIT)) {
        Node *n = node_new(AST_INT_LIT, p->cur.line);
        n->ival = p->cur.ival;
        advance(p);
        return n;
    }

    /* Identifier or function call */
    if (check(p, TOK_IDENT)) {
        Token ident_tok = p->cur;         /* save before consuming */
        advance(p);                       /* consume the identifier */

        if (match(p, TOK_LPAREN)) {
            /* Function call: IDENT "(" arglist ")" */
            Node *n  = node_new(AST_CALL, ident_tok.line);
            n->sval  = my_strndup(ident_tok.start, ident_tok.len);
            n->args  = NULL;
            n->nargs = 0;

            if (!check(p, TOK_RPAREN)) {
                do {
                    Node *arg = parse_expr(p);
                    n->args = realloc(n->args,
                                      (size_t)(n->nargs + 1) * sizeof(Node *));
                    if (!n->args) {
                        fprintf(stderr, "out of memory in parse_primary (args)\n");
                        exit(1);
                    }
                    n->args[n->nargs++] = arg;
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, "')' after argument list");
            return n;
        }

        /* Plain identifier reference */
        Node *n = node_new(AST_IDENT, ident_tok.line);
        n->sval = my_strndup(ident_tok.start, ident_tok.len);
        return n;
    }

    /* Parenthesised expression */
    if (match(p, TOK_LPAREN)) {
        Node *inner = parse_expr(p);
        expect(p, TOK_RPAREN, "')' after expression");
        return inner;
    }

    /* Nothing matched */
    parse_error(p, "expected an expression");
    return NULL;  /* unreachable, but silences compiler warning */
}
