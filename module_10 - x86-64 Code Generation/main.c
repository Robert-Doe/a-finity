/* main.c — Driver for my_compiler Module 10: Code Generation
 *
 * Usage:
 *   ./codegen input.c              # emit assembly to stdout
 *   ./codegen input.c -o out.asm   # emit assembly to file
 *
 * Pipeline:
 *   Source -> Lexer -> Parser -> Sema -> IR -> Opt -> CodeGen -> NASM
 */
#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s input.c [-o output.asm]\n", argv[0]);
        return 1;
    }

    const char *input_file  = argv[1];
    const char *output_file = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    /* ---- 1. Source ---- */
    Source *src = source_open(input_file);
    if (!src) return 1;

    /* ---- 2. Lexer ---- */
    Lexer lx;
    lexer_init(&lx, src);

    /* ---- 3. Parser ---- */
    Parser p;
    parser_init(&p, &lx);
    Node *ast = parse_program(&p);
    if (!ast) {
        fprintf(stderr, "error: parsing failed\n");
        source_free(src);
        return 1;
    }

    /* ---- 4. Semantic analysis ---- */
    int sema_errors = sema_check(ast);
    if (sema_errors) {
        fprintf(stderr, "error: %d semantic error(s)\n", sema_errors);
        node_free(ast);
        source_free(src);
        return 1;
    }

    /* ---- 5. IR generation ---- */
    IRProg *prog = ir_gen(ast);
    node_free(ast);
    source_free(src);

    /* ---- 6. Optimisation ---- */
    opt_all(prog);

    /* ---- 7. Code generation ---- */
    FILE *out = stdout;
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "error: cannot open output file '%s'\n", output_file);
            ir_prog_free(prog);
            return 1;
        }
    }

    codegen(prog, out);

    if (output_file) {
        fclose(out);
        fprintf(stderr, "assembly written to '%s'\n", output_file);
    }

    ir_prog_free(prog);
    return 0;
}
