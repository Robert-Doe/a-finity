/* regalloc.c — Register allocator (stack-slot) implementation */
#include "regalloc.h"

#include <string.h>
#include <stdio.h>

/* Assign a new slot and return its number (1-based). */
static int new_slot(RegMap *rm) {
    return ++rm->n_slots;
}

/* Find or create a named-local entry; return its slot. */
static int get_named_slot(RegMap *rm, const char *name) {
    for (int i = 0; i < rm->n_named; i++)
        if (strcmp(rm->named_name[i], name) == 0)
            return rm->named_slot[i];
    /* Not found: allocate a new slot. */
    if (rm->n_named >= REGMAP_MAX) return 1; /* guard */
    int slot = new_slot(rm);
    rm->named_slot[rm->n_named] = slot;
    strncpy(rm->named_name[rm->n_named], name, 63);
    rm->named_name[rm->n_named][63] = '\0';
    rm->n_named++;
    return slot;
}

void regmap_build(RegMap *rm, const IRFunc *f) {
    memset(rm, 0, sizeof(*rm));

    /* First pass: allocate slots for all named locals (STORE/LOAD names). */
    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if ((ins->op == IR_STORE || ins->op == IR_LOAD) && ins->name[0])
            get_named_slot(rm, ins->name);
    }

    /* Second pass: allocate slots for all temporaries. */
    for (IRInstr *ins = f->head; ins; ins = ins->next) {
        if (ins->dst >= 0 && ins->dst < REGMAP_MAX && rm->temp_slot[ins->dst] == 0)
            rm->temp_slot[ins->dst] = new_slot(rm);
    }
}

int regmap_temp_slot(const RegMap *rm, int t) {
    if (t < 0 || t >= REGMAP_MAX) return 1;
    return rm->temp_slot[t] ? rm->temp_slot[t] : 1;
}

int regmap_named_slot(const RegMap *rm, const char *name) {
    for (int i = 0; i < rm->n_named; i++)
        if (strcmp(rm->named_name[i], name) == 0)
            return rm->named_slot[i];
    return 0;
}
