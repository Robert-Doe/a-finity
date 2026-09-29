/* ir.c — IR generation implementation for my_compiler
 * Module 08: IR Generation
 * Prerequisites: source.h, token.h, lexer.h, symtab.h, ast.h, parser.h, sema.h, ir.h
 */
#include "ir.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helper: ir_new_instr                                         */
/* ------------------------------------------------------------------ */

/*
 * Allocate a new IRInstr on the heap and initialise all fields safely.
 *
 * We use memset to zero every byte first (sets the linked-list pointer
 * `next` to NULL, the name[] array to "", and nargs/ival to 0).
 *
 * IMPORTANT: IR_NO_TEMP is -1, NOT 0.  memset(0) would leave dst/src1/src2
 * as 0, which looks like "temporary t0" — wrong.  So after memset we
 * explicitly set dst, src1, src2 to IR_NO_TEMP.  Callers override whichever
 * fields they actually use.
 */
static IRInstr *ir_new_instr(IROp op) {
    IRInstr *instr = malloc(sizeof(IRInstr));
    if (!instr) { fprintf(stderr, "ir: out of memory\n"); exit(1); }
    memset(instr, 0, sizeof(IRInstr)); /* zero name[], next, ival, nargs */
    instr->op   = op;
    instr->dst  = IR_NO_TEMP; /* default: this instruction produces no value */
    instr->src1 = IR_NO_TEMP; /* default: no first source operand */
    instr->src2 = IR_NO_TEMP; /* default: no second source operand */
    return instr;
}

/* ------------------------------------------------------------------ */
/* Internal helper: ir_append                                            */
/* ------------------------------------------------------------------ */

/*
 * Append instr to the end of function f's instruction list.
 *
 * We maintain f->tail so that each append is O(1) — no need to walk
 * the entire list to find the last element.  Without tail, appending N
 * instructions would be O(N^2) overall.
 */
static void ir_append(IRFunc *f, IRInstr *instr) {
    if (!f->head) {
        /* List is empty: both head and tail point to the first node */
        f->head = f->tail = instr;
    } else {
        /* Link the current tail to the new node, then advance tail */
        f->tail->next = instr;
        f->tail       = instr;
    }
    instr->next = NULL; /* new tail has no successor */
}

/* ------------------------------------------------------------------ */
/* Internal helper: ir_new_temp                                          */
/* ------------------------------------------------------------------ */

/*
 * Allocate the next available temporary register number for function f.
 * Temporaries are named t0, t1, t2, … at the printing stage.
 * This counter never resets within a function (SSA-like property without
 * full SSA: each temp is written exactly once because we generate in a
 * single forward pass).
 */
static int ir_new_temp(IRFunc *f) {
    return f->next_temp++; /* return current value, then increment */
}

/* ------------------------------------------------------------------ */
/* Internal helper: ir_new_label                                         */
/* ------------------------------------------------------------------ */

/*
 * Allocate the next label number for function f.
 * Labels are printed as "L0", "L1", etc.  Having a per-function counter
 * means labels are unique within a function; we embed the function name
 * in the label string to make them unique across the whole program.
 */
static int ir_new_label(IRFunc *f) {
    return f->next_label++; /* return current value, then increment */
}

/* ------------------------------------------------------------------ */
/* gen_expr — generate IR for an expression node                         */
/* Returns the temporary number that holds the result.                   */
/* ------------------------------------------------------------------ */
static int gen_expr(IRFunc *f, Node *n);

/* ------------------------------------------------------------------ */
/* gen_stmt — generate IR for a statement node                           */
/* ------------------------------------------------------------------ */
static void gen_stmt(IRFunc *f, Node *n);

static int gen_expr(IRFunc *f, Node *n) {
    if (!n) return IR_NO_TEMP;

    switch (n->kind) {

    /* -------------------------------------------------------------- */
    case AST_INT_LIT: {
        /* Emit: dst = ICONST ival */
        int dst = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(IR_ICONST);
        instr->dst  = dst;
        instr->ival = n->ival; /* the literal integer value */
        ir_append(f, instr);
        return dst;
    }

    /* -------------------------------------------------------------- */
    case AST_IDENT: {
        /* Emit: dst = LOAD name  (read from local variable storage) */
        int dst = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(IR_LOAD);
        instr->dst = dst;
        strncpy(instr->name, n->name, sizeof(instr->name) - 1);
        ir_append(f, instr);
        return dst;
    }

    /* -------------------------------------------------------------- */
    case AST_UNARY: {
        /* Only unary '-' is supported in this grammar */
        int operand = gen_expr(f, n->args[0]); /* generate operand first */
        int dst = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(IR_NEG);
        instr->dst  = dst;
        instr->src1 = operand;
        ir_append(f, instr);
        return dst;
    }

    /* -------------------------------------------------------------- */
    case AST_BINARY: {
        /* Map the source-level operator token to an IROp */
        IROp op;
        switch (n->op) {
            case TOK_PLUS:    op = IR_ADD; break;
            case TOK_MINUS:   op = IR_SUB; break;
            case TOK_STAR:    op = IR_MUL; break;
            case TOK_SLASH:   op = IR_DIV; break;
            case TOK_PERCENT: op = IR_MOD; break;
            case TOK_LT:      op = IR_LT;  break;
            case TOK_GT:      op = IR_GT;  break;
            case TOK_LEQ:     op = IR_LEQ; break;
            case TOK_GEQ:     op = IR_GEQ; break;
            case TOK_EQEQ:    op = IR_EQ;  break;
            case TOK_NEQ:     op = IR_NEQ; break;
            case TOK_AND:     op = IR_AND; break;
            case TOK_OR:      op = IR_OR;  break;
            default:
                fprintf(stderr, "ir: unknown binary op %d\n", n->op);
                return IR_NO_TEMP;
        }
        /* Evaluate left then right (left-to-right evaluation order) */
        int left  = gen_expr(f, n->args[0]);
        int right = gen_expr(f, n->args[1]);
        int dst   = ir_new_temp(f);
        IRInstr *instr = ir_new_instr(op);
        instr->dst  = dst;
        instr->src1 = left;
        instr->src2 = right;
        ir_append(f, instr);
        return dst;
    }

    /* -------------------------------------------------------------- */
    case AST_ASSIGN: {
        /* Evaluate the right-hand side expression */
        int val = gen_expr(f, n->args[0]);
        /* Emit: STORE name = val  (write back to the named local) */
        IRInstr *store = ir_new_instr(IR_STORE);
        store->src1 = val;
        strncpy(store->name, n->name, sizeof(store->name) - 1);
        ir_append(f, store);
        return val; /* assignment also yields the value (for chained assignment) */
    }

    /* -------------------------------------------------------------- */
    case AST_CALL: {
        /* Evaluate and emit IR_PARAM for each argument in order */
        for (int i = 0; i < n->n_args; i++) {
            int arg = gen_expr(f, n->args[i]); /* evaluate ith argument */
            IRInstr *param = ir_new_instr(IR_PARAM);
            param->src1 = arg;                 /* the temp holding the argument */
            ir_append(f, param);
        }
        /* Emit the call instruction; dst receives the return value */
        int dst = ir_new_temp(f);
        IRInstr *call = ir_new_instr(IR_CALL);
        call->dst   = dst;
        call->nargs = n->n_args;
        strncpy(call->name, n->name, sizeof(call->name) - 1);
        ir_append(f, call);
        return dst;
    }

    default:
        fprintf(stderr, "ir: gen_expr: unhandled node kind %d\n", n->kind);
        return IR_NO_TEMP;
    }
}

static void gen_stmt(IRFunc *f, Node *n) {
    if (!n) return;

    switch (n->kind) {

    /* -------------------------------------------------------------- */
    case AST_RETURN: {
        int val = IR_NO_TEMP;
        if (n->n_args > 0) {
            val = gen_expr(f, n->args[0]); /* evaluate the return expression */
        }
        IRInstr *ret = ir_new_instr(IR_RETURN);
        ret->src1 = val; /* IR_NO_TEMP signals a void return */
        ir_append(f, ret);
        break;
    }

    /* -------------------------------------------------------------- */
    case AST_IF: {
        /*
         * Pattern for if (cond) then [else alt]:
         *   <evaluate cond into t_cond>
         *   JUMPZ t_cond, else_label      ; jump to else if cond is false (0)
         *   <then branch instructions>
         *   JUMP  end_label               ; skip the else branch
         *   LABEL else_label
         *   <else branch instructions (if any)>
         *   LABEL end_label
         */
        int else_lbl = ir_new_label(f);
        int end_lbl  = ir_new_label(f);
        char else_name[64], end_name[64];
        snprintf(else_name, sizeof(else_name), "L%d", else_lbl);
        snprintf(end_name,  sizeof(end_name),  "L%d", end_lbl);

        /* Evaluate condition */
        int cond = gen_expr(f, n->args[0]);

        /* If cond == 0 jump to else branch */
        IRInstr *jumpz = ir_new_instr(IR_JUMPZ);
        jumpz->src1 = cond;
        strncpy(jumpz->name, else_name, sizeof(jumpz->name) - 1);
        ir_append(f, jumpz);

        /* Then branch */
        gen_stmt(f, n->args[1]);

        /* After then, jump over the else branch */
        IRInstr *jump = ir_new_instr(IR_JUMP);
        strncpy(jump->name, end_name, sizeof(jump->name) - 1);
        ir_append(f, jump);

        /* else_label: beginning of else branch (or merge point if no else) */
        IRInstr *lbl_else = ir_new_instr(IR_LABEL);
        strncpy(lbl_else->name, else_name, sizeof(lbl_else->name) - 1);
        ir_append(f, lbl_else);

        /* Else branch (optional) */
        if (n->n_args > 2) {
            gen_stmt(f, n->args[2]);
        }

        /* end_label: merge point after both branches */
        IRInstr *lbl_end = ir_new_instr(IR_LABEL);
        strncpy(lbl_end->name, end_name, sizeof(lbl_end->name) - 1);
        ir_append(f, lbl_end);
        break;
    }

    /* -------------------------------------------------------------- */
    case AST_WHILE: {
        /*
         * Pattern for while (cond) body:
         *   LABEL loop_label              ; top of loop
         *   <evaluate cond into t_cond>
         *   JUMPZ t_cond, end_label       ; exit loop if cond is false
         *   <body instructions>
         *   JUMP  loop_label              ; go back to top
         *   LABEL end_label               ; after the loop
         */
        int loop_lbl = ir_new_label(f);
        int end_lbl  = ir_new_label(f);
        char loop_name[64], end_name[64];
        snprintf(loop_name, sizeof(loop_name), "L%d", loop_lbl);
        snprintf(end_name,  sizeof(end_name),  "L%d", end_lbl);

        /* Top-of-loop label */
        IRInstr *lbl_loop = ir_new_instr(IR_LABEL);
        strncpy(lbl_loop->name, loop_name, sizeof(lbl_loop->name) - 1);
        ir_append(f, lbl_loop);

        /* Evaluate condition and branch if false */
        int cond = gen_expr(f, n->args[0]);
        IRInstr *jumpz = ir_new_instr(IR_JUMPZ);
        jumpz->src1 = cond;
        strncpy(jumpz->name, end_name, sizeof(jumpz->name) - 1);
        ir_append(f, jumpz);

        /* Loop body */
        gen_stmt(f, n->args[1]);

        /* Jump back to evaluate the condition again */
        IRInstr *jump = ir_new_instr(IR_JUMP);
        strncpy(jump->name, loop_name, sizeof(jump->name) - 1);
        ir_append(f, jump);

        /* After-loop label */
        IRInstr *lbl_end = ir_new_instr(IR_LABEL);
        strncpy(lbl_end->name, end_name, sizeof(lbl_end->name) - 1);
        ir_append(f, lbl_end);
        break;
    }

    /* -------------------------------------------------------------- */
    case AST_BLOCK:
        /* A block is just a sequence of statements — recurse on each */
        for (int i = 0; i < n->n_args; i++) {
            gen_stmt(f, n->args[i]);
        }
        break;

    /* -------------------------------------------------------------- */
    case AST_VAR_DECL:
        /*
         * Count this local so the code generator knows how much stack
         * space to reserve (used in later modules for stack layout).
         */
        f->n_locals++;
        /* If there is an initialiser, evaluate it and store to the variable */
        if (n->n_args > 0) {
            int val = gen_expr(f, n->args[0]);
            IRInstr *store = ir_new_instr(IR_STORE);
            store->src1 = val;
            strncpy(store->name, n->name, sizeof(store->name) - 1);
            ir_append(f, store);
        }
        break;

    /* -------------------------------------------------------------- */
    case AST_EXPR_STMT:
        /* Generate the expression for its side-effects; discard the result */
        gen_expr(f, n->args[0]);
        break;

    /* -------------------------------------------------------------- */
    case AST_ASSIGN:
        /* An assignment can also appear as a statement (wraps AST_ASSIGN expr) */
        gen_expr(f, n);
        break;

    default:
        fprintf(stderr, "ir: gen_stmt: unhandled node kind %d\n", n->kind);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* gen_function — set up one IRFunc and generate its body               */
/* ------------------------------------------------------------------ */
static void gen_function(IRProg *prog, Node *func_node) {
    if (prog->n_funcs >= 32) {
        fprintf(stderr, "ir: too many functions (max 32)\n");
        return;
    }

    IRFunc *f = &prog->funcs[prog->n_funcs++]; /* take the next IRFunc slot */
    memset(f, 0, sizeof(IRFunc));               /* zero all counters */
    strncpy(f->name, func_node->name, sizeof(f->name) - 1);

    /*
     * Register function parameters as locals with IR_STORE.
     * In our IR, parameters arrive via the calling convention (set up by
     * the caller's IR_PARAM instructions).  We model each parameter as a
     * named local so that IR_LOAD/IR_STORE work uniformly for both
     * parameters and local variables.
     *
     * The AST_FUNC node stores parameters as args[0..n_params-1] and the
     * body block as args[n_params] (the last child).
     */
    int body_idx = func_node->n_args - 1; /* index of the body block */

    /* Body is the last child; generate all statements in it */
    gen_stmt(f, func_node->args[body_idx]);
}

/* ------------------------------------------------------------------ */
/* irgen — entry point: generate IR for the whole program               */
/* ------------------------------------------------------------------ */
IRProg *irgen(Node *program) {
    IRProg *prog = malloc(sizeof(IRProg));
    if (!prog) { fprintf(stderr, "ir: out of memory\n"); exit(1); }
    memset(prog, 0, sizeof(IRProg)); /* zero n_funcs and all IRFunc slots */

    /* Generate IR for each top-level function */
    for (int i = 0; i < program->n_args; i++) {
        if (program->args[i]->kind == AST_FUNC) {
            gen_function(prog, program->args[i]);
        }
    }
    return prog;
}

/* ------------------------------------------------------------------ */
/* ir_print — human-readable dump of the IR                             */
/* ------------------------------------------------------------------ */

/* Return a string name for an IROp. */
static const char *op_name(IROp op) {
    switch (op) {
        case IR_ICONST: return "ICONST";
        case IR_COPY:   return "COPY";
        case IR_ADD:    return "ADD";
        case IR_SUB:    return "SUB";
        case IR_MUL:    return "MUL";
        case IR_DIV:    return "DIV";
        case IR_MOD:    return "MOD";
        case IR_NEG:    return "NEG";
        case IR_LT:     return "LT";
        case IR_GT:     return "GT";
        case IR_LEQ:    return "LEQ";
        case IR_GEQ:    return "GEQ";
        case IR_EQ:     return "EQ";
        case IR_NEQ:    return "NEQ";
        case IR_AND:    return "AND";
        case IR_OR:     return "OR";
        case IR_LABEL:  return "LABEL";
        case IR_JUMP:   return "JUMP";
        case IR_JUMPZ:  return "JUMPZ";
        case IR_PARAM:  return "PARAM";
        case IR_CALL:   return "CALL";
        case IR_RETURN: return "RETURN";
        case IR_STORE:  return "STORE";
        case IR_LOAD:   return "LOAD";
        default:        return "???";
    }
}

void ir_print(const IRProg *prog) {
    for (int fi = 0; fi < prog->n_funcs; fi++) {
        const IRFunc *f = &prog->funcs[fi];
        printf("=== IR: %s ===\n", f->name);

        for (IRInstr *instr = f->head; instr; instr = instr->next) {
            printf("  "); /* indent all instructions */

            switch (instr->op) {
                case IR_ICONST:
                    printf("t%d = ICONST %ld\n", instr->dst, instr->ival);
                    break;
                case IR_COPY:
                    printf("t%d = COPY t%d\n", instr->dst, instr->src1);
                    break;
                case IR_NEG:
                    printf("t%d = NEG t%d\n", instr->dst, instr->src1);
                    break;
                case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
                case IR_LT:  case IR_GT:  case IR_LEQ: case IR_GEQ:
                case IR_EQ:  case IR_NEQ: case IR_AND: case IR_OR:
                    printf("t%d = %s t%d t%d\n",
                           instr->dst, op_name(instr->op), instr->src1, instr->src2);
                    break;
                case IR_LOAD:
                    printf("t%d = LOAD %s\n", instr->dst, instr->name);
                    break;
                case IR_STORE:
                    printf("STORE %s = t%d\n", instr->name, instr->src1);
                    break;
                case IR_LABEL:
                    printf("LABEL %s\n", instr->name);
                    break;
                case IR_JUMP:
                    printf("JUMP %s\n", instr->name);
                    break;
                case IR_JUMPZ:
                    printf("JUMPZ t%d %s\n", instr->src1, instr->name);
                    break;
                case IR_PARAM:
                    printf("PARAM t%d\n", instr->src1);
                    break;
                case IR_CALL:
                    printf("t%d = CALL %s (%d args)\n",
                           instr->dst, instr->name, instr->nargs);
                    break;
                case IR_RETURN:
                    if (instr->src1 == IR_NO_TEMP)
                        printf("RETURN\n");
                    else
                        printf("RETURN t%d\n", instr->src1);
                    break;
                default:
                    printf("%s\n", op_name(instr->op));
                    break;
            }
        }
        printf("\n");
    }
}

/* ------------------------------------------------------------------ */
/* ir_free — release all memory allocated by irgen                      */
/* ------------------------------------------------------------------ */
void ir_free(IRProg *prog) {
    if (!prog) return;
    for (int fi = 0; fi < prog->n_funcs; fi++) {
        /* Walk the linked list and free each instruction node */
        IRInstr *cur = prog->funcs[fi].head;
        while (cur) {
            IRInstr *next = cur->next;
            free(cur);      /* free the instruction */
            cur = next;     /* advance before the pointer becomes invalid */
        }
    }
    free(prog); /* free the IRProg struct itself */
}
