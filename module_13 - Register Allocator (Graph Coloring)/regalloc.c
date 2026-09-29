/* regalloc.c — Register allocation via graph coloring for bob_compiler
 * Module 13: Register Allocation
 *
 * Algorithm
 * ---------
 *   Step 1  Number every instruction in the function (index 0, 1, 2, ...).
 *   Step 2  Liveness analysis:
 *             first_def[t] = index of the instruction that defines t (writes dst).
 *             last_use[t]  = index of the last instruction that uses t (reads src*).
 *           A temporary's live range is [first_def[t], last_use[t]].
 *   Step 3  Interference: t1 and t2 interfere iff their live ranges overlap:
 *             first_def[t1] <= last_use[t2]  &&  first_def[t2] <= last_use[t1]
 *   Step 4  Greedy coloring in first_def order:
 *             For each temp t (sorted ascending by first_def):
 *               Collect the set of colors used by already-colored interfering temps.
 *               Pick the lowest color 0..NUM_REGS-1 not in that set.
 *               If no color is available: spill — assign a stack slot.
 */
#include "regalloc.h"

#include <stdio.h>
#include <string.h>

/* The six registers we hand out. */
const char *REG_NAMES[NUM_REGS] = {
    "rbx", "r12", "r13", "r14", "r15", "r10"
};

/* Spill slots start at -8 relative to rbp (below the local variables area).
 * The codegen will add extra space in the prologue for these.
 * We use a base offset of 0 here; codegen combines n_locals and n_spilled. */
#define SPILL_BASE_BYTES 8  /* first spill slot = -8 from rbp (adjusted by codegen) */

/* ------------------------------------------------------------------ */
/* Step 1+2: Number instructions and compute live ranges               */
/* ------------------------------------------------------------------ */

static void compute_live_ranges(const IRFunc *fn,
                                 int first_def[MAX_TEMPS],
                                 int last_use[MAX_TEMPS],
                                 int *n_temps_out,
                                 int *n_instrs_out) {
    /* Initialise: -1 means "not yet seen". */
    for (int i = 0; i < MAX_TEMPS; i++) {
        first_def[i] = -1;
        last_use[i]  = -1;
    }

    int idx = 0;   /* instruction index */
    int max_t = 0; /* highest temp seen */

    for (const IRInstr *ins = fn->head; ins; ins = ins->next, idx++) {
        /* Record definition (write to dst). */
        if (ins->dst != IR_NO_TEMP && ins->dst < MAX_TEMPS) {
            int t = ins->dst;
            if (first_def[t] < 0) first_def[t] = idx; /* first write */
            if (last_use[t]  < idx) last_use[t] = idx; /* also a use point */
            if (t + 1 > max_t) max_t = t + 1;
        }
        /* Record uses (reads of src1, src2). */
        if (ins->src1 != IR_NO_TEMP && ins->src1 < MAX_TEMPS) {
            int t = ins->src1;
            if (first_def[t] < 0) first_def[t] = idx; /* defensive: define if unseen */
            if (last_use[t]  < idx) last_use[t] = idx;
            if (t + 1 > max_t) max_t = t + 1;
        }
        if (ins->src2 != IR_NO_TEMP && ins->src2 < MAX_TEMPS) {
            int t = ins->src2;
            if (first_def[t] < 0) first_def[t] = idx;
            if (last_use[t]  < idx) last_use[t] = idx;
            if (t + 1 > max_t) max_t = t + 1;
        }
    }

    *n_temps_out  = max_t;
    *n_instrs_out = idx;
}

/* ------------------------------------------------------------------ */
/* Step 3+4: Greedy graph coloring                                     */
/* ------------------------------------------------------------------ */

/*
 * Two live ranges [a,b] and [c,d] overlap iff a <= d && c <= b.
 * We only check temps that have actually been defined (first_def >= 0).
 */
static int ranges_overlap(int a, int b, int c, int d) {
    return (a <= d) && (c <= b);
}

RegMap regalloc(const IRFunc *fn) {
    RegMap rm;
    memset(&rm, 0, sizeof(rm));
    for (int i = 0; i < MAX_TEMPS; i++) {
        rm.assign[i]     = -1;  /* -1 = uncolored / spilled */
        rm.spill_slot[i] = 0;
    }
    rm.n_spilled = 0;

    /* Step 1+2: live ranges */
    int first_def[MAX_TEMPS];
    int last_use[MAX_TEMPS];
    int n_temps  = 0;
    int n_instrs = 0;
    compute_live_ranges(fn, first_def, last_use, &n_temps, &n_instrs);

    if (n_temps == 0) return rm; /* empty function */

    /*
     * Step 3+4: greedy coloring in first_def order.
     *
     * We process temps 0, 1, 2, … in order.  Because the IR generator
     * allocates temps in definition order, this is equivalent to processing
     * in first_def order.
     */
    for (int t = 0; t < n_temps; t++) {
        if (first_def[t] < 0) continue; /* temp never defined, skip */

        /* Collect colors already used by interfering, already-colored temps. */
        int used[NUM_REGS];
        memset(used, 0, sizeof(used));

        for (int other = 0; other < t; other++) {
            if (first_def[other] < 0) continue;
            if (rm.assign[other] < 0) continue; /* other is spilled, no color */

            /* Do the live ranges of t and other overlap? */
            if (ranges_overlap(first_def[t],     last_use[t],
                               first_def[other], last_use[other])) {
                int color = rm.assign[other];
                if (color >= 0 && color < NUM_REGS)
                    used[color] = 1;
            }
        }

        /* Assign the lowest available color. */
        int chosen = -1;
        for (int c = 0; c < NUM_REGS; c++) {
            if (!used[c]) { chosen = c; break; }
        }

        if (chosen >= 0) {
            rm.assign[t] = chosen;
        } else {
            /* All registers taken: spill this temp. */
            rm.assign[t]     = -1;
            rm.spill_slot[t] = -(SPILL_BASE_BYTES + rm.n_spilled * 8);
            rm.n_spilled++;
        }
    }

    return rm;
}

/* ------------------------------------------------------------------ */
/* regmap_print                                                         */
/* ------------------------------------------------------------------ */

void regmap_print(const RegMap *rm, int n_temps) {
    printf("=== Register Map ===\n");
    for (int t = 0; t < n_temps && t < MAX_TEMPS; t++) {
        if (rm->assign[t] >= 0) {
            printf("  t%-3d  -> %s\n", t, REG_NAMES[rm->assign[t]]);
        } else {
            printf("  t%-3d  -> [rbp%d]  (spilled)\n", t, rm->spill_slot[t]);
        }
    }
    printf("  (%d spilled)\n\n", rm->n_spilled);
}
