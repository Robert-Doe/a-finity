/* main.c — Driver for my_compiler (Module 14: ELF Object File Emitter)
 * Usage: ./mycc14 input.c -o output.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"
#include "codegen.h"
#include "elf_emit.h"

int main(int argc, char *argv[]) {
    if (argc != 4 || strcmp(argv[2], "-o") != 0) {
        fprintf(stderr, "usage: %s input.c -o output.o\n", argv[0]);
        return 1;
    }
    const char *input_path  = argv[1];
    const char *output_path = argv[3];

    /* 1. Source */
    Source *src = source_open(input_path);
    if (!src) return 1;

    /* 2. Lex */
    Lexer lx;
    lexer_init(&lx, src);

    /* 3. Parse */
    Parser p;
    parser_init(&p, &lx);
    Node *prog = parse_program(&p);
    if (p.had_error) {
        fprintf(stderr, "parse errors; aborting.\n");
        return 1;
    }

    /* 4. Semantic analysis */
    if (sema_check(prog) != 0) {
        fprintf(stderr, "semantic errors; aborting.\n");
        return 1;
    }

    /* 5. IR generation */
    IRProg *ir = ir_gen(prog);

    /* 6. Optimization */
    opt_all(ir);

    /* 7. Code generation → ELF */
    ElfEmitter e;
    elf_init(&e);
    codegen_program(ir, &e);

    /* 8. Write ELF object file */
    elf_write(&e, output_path);

    printf("compiled %d function%s to %s\n",
           ir->n_funcs,
           ir->n_funcs == 1 ? "" : "s",
           output_path);

    ir_prog_free(ir);
    source_free(src);
    return 0;
}
