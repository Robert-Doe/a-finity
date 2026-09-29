/* main.c — Module 04 demo: expression parser
 * Parses several test expressions and prints their AST.
 *
 * No file I/O needed — we build in-memory Sources from string literals.
 */
#include <stdio.h>
#include <string.h>

#include "source.h"
#include "parser.h"
#include "ast.h"

/* source_from_string: create a Source that wraps a string literal.
 *
 * IMPORTANT: this Source does NOT own its memory (the text field points
 * into a string literal in read-only memory).  Never call source_free()
 * on a Source produced by this function.
 *
 * Params:  text — NUL-terminated C string to scan
 *          name — filename shown in error messages
 * Returns: a Source ready to hand to parser_init
 */
static Source source_from_string(const char *text, const char *name) {
    Source src;
    src.text     = (char *)text;  /* safe: lexer only reads, never writes */
    src.len      = strlen(text);
    src.filename = name;
    return src;
}

/* parse_and_print: parse one expression string and pretty-print its AST.
 *
 * Params:  input — the expression source text
 *          label — description printed as a header
 */
static void parse_and_print(const char *input, const char *label) {
    printf("\nInput: \"%s\"\n", label);  /* print the human-readable label */

    Source src = source_from_string(input, "<test>");
    Parser p;
    parser_init(&p, &src);              /* initialise parser (loads first token) */

    Node *tree = parse_expr(&p);        /* parse one expression */

    if (p.had_error || !tree) {
        printf("  (parse error)\n");
    } else {
        node_print(tree, 1);            /* indent by 1 level for readability */
    }

    node_free(tree);                    /* release AST memory */
    /* NOTE: do NOT call source_free(&src) — we didn't heap-allocate the text */
}

int main(void) {
    printf("=== Expression Parser — Module 04 ===\n");

    /* Test 1: operator precedence — * binds tighter than + */
    parse_and_print("3 + 4 * 2",
                    "3 + 4 * 2");

    /* Test 2: comparison and logical operators */
    parse_and_print("a == b && c > 0",
                    "a == b && c > 0");

    /* Test 3: function call with an expression argument */
    parse_and_print("foo(1, 2 + 3)",
                    "foo(1, 2 + 3)");

    /* Test 4: unary minus wrapping a parenthesised expression */
    parse_and_print("-(x + 1)",
                    "-(x + 1)");

    printf("\n");
    return 0;
}
