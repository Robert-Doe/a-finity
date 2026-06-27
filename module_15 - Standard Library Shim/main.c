/* main.c — Compiler driver for my_compiler
 * Module 15: Standard Library Shim
 *
 * Usage:
 *   ./mycc15 input.c -o output.asm
 *
 * Pipeline:
 *   source -> lexer -> parser -> sema -> irgen -> opt -> codegen -> .asm
 *
 * After writing output.asm, the driver prints instructions showing how to
 * assemble and link the output together with runtime.asm.
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
    printf("=== my_compiler Module 15: Standard Library Shim ===\n\n");

    /* Parse arguments: mycc15 <input.c> -o <output.asm> */
    const char *input_file  = NULL;
    const char *output_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "usage: %s <source.c> -o <output.asm>\n", argv[0]);
        return 1;
    }

    /* Step 1: Open the source file */
    Source *src = source_open(input_file);
    if (!src) return 1;

    /* Step 2: Lex + parse -> AST */
    Lexer  lx;
    Parser p;
    lexer_init(&lx, src);
    Node *program = parse_program(&p, &lx);
    if (!program || p.had_error) {
        fprintf(stderr, "parse errors -- aborting\n");
        source_free(src);
        return 1;
    }

    /* Step 3: Semantic analysis */
    Sema sema;
    if (sema_check(&sema, program)) {
        fprintf(stderr, "semantic errors -- aborting\n");
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

    /* Step 6: Open output file (or stdout) */
    FILE *out = stdout;
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "error: cannot open '%s' for writing\n", output_file);
            ir_free(ir);
            node_free(program);
            source_free(src);
            return 1;
        }
        printf("Writing NASM assembly to: %s\n\n", output_file);
    } else {
        printf("--- NASM Assembly Output ---\n");
    }

    /* Step 7: Generate NASM assembly */
    codegen(ir, out);

    if (out != stdout) {
        fclose(out);
        printf("Done.\n\n");
        printf("To assemble and run (Linux / WSL):\n");
        printf("  nasm -f elf64 %s -o prog.o\n", output_file);
        printf("  nasm -f elf64 runtime.asm -o runtime.o\n");
        printf("  gcc -no-pie prog.o runtime.o -o program\n");
        printf("  ./program\n");
    }

    /* Step 8: Clean up */
    ir_free(ir);
    node_free(program);
    source_free(src);

    return 0;
}
