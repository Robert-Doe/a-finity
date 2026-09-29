/* main.c — Compiler driver for my_compiler
 * Module 12: Function Calls and the System V AMD64 Calling Convention
 *
 * Usage:
 *   ./codegen12 <source.c>              -- write assembly to stdout
 *   ./codegen12 <source.c> <out.s>     -- write assembly to file
 *
 * Pipeline:
 *   source -> lexer -> parser -> sema -> irgen -> opt -> codegen -> .s
 *
 * To compile and run the generated assembly (Linux/WSL):
 *   ./codegen12 sample.c sample.s
 *   gcc -no-pie -o sample sample.s
 *   ./sample ; echo "exit code: $?"
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

int main(int argc, char *argv[]) {
    printf("=== my_compiler Module 12: ABI-Compliant Codegen ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "usage: %s <source_file> [output.s]\n", argv[0]);
        return 1;
    }

    /* Step 1: Open the source file */
    Source *src = source_open(argv[1]);
    if (!src) return 1;

    /* Step 2: Lex + parse -> AST */
    Lexer  lx;
    Parser p;
    lexer_init(&lx, src);
    Node *program = parse_program(&p, &lx);
    if (!program || p.had_error) {
        fprintf(stderr, "parse errors — aborting\n");
        source_free(src);
        return 1;
    }

    /* Step 3: Semantic analysis */
    Sema sema;
    if (sema_check(&sema, program)) {
        fprintf(stderr, "semantic errors — aborting\n");
        node_free(program);
        source_free(src);
        return 1;
    }

    /* Step 4: Generate IR */
    printf("--- IR (before optimisation) ---\n");
    IRProg *ir = irgen(program);
    ir_print(ir);

    /* Step 5: Optimise */
    opt_run(ir);

    /* Step 6: Generate x86-64 assembly */
    FILE *out = stdout;
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) {
            fprintf(stderr, "error: cannot open '%s' for writing\n", argv[2]);
            ir_free(ir);
            node_free(program);
            source_free(src);
            return 1;
        }
        printf("Writing assembly to: %s\n\n", argv[2]);
    } else {
        printf("--- Assembly Output ---\n");
    }

    codegen(ir, out);

    if (out != stdout) {
        fclose(out);
        printf("Done.  To run:\n");
        printf("  gcc -no-pie -o out %s\n", argv[2]);
        printf("  ./out ; echo \"exit code: $?\"\n");
    }

    /* Step 7: Clean up */
    ir_free(ir);
    node_free(program);
    source_free(src);

    return 0;
}
