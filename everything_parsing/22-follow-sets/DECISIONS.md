# Module 22 — Decisions

Choices in `FollowSets.java` / `followsets.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The three rules

### FOLLOW is defined by exactly these three rules — (b)
Straight from the Dragon Book (§4.4.2) and CSE 340:

1. `$ ∈ FOLLOW(start)`.
2. `B → α A β` ⇒ `FIRST(β) \ {ε} ⊆ FOLLOW(A)`.
3. `B → α A β` with `β` nullable (β empty counts) ⇒ `FOLLOW(B) ⊆ FOLLOW(A)`.

We don't invent or simplify these. Rule 3 is the one people forget; without it
`FOLLOW(Ep)` in the expression grammar never picks up `)`.

### `$` is seeded, not special-cased later — (a)
`FOLLOW(start)` starts as `{$}` before the iteration. The alternative — leaving
FOLLOW sets `$`-free and having the parser driver "just know" about EOF — breaks
the PREDICT computation: a nullable production that can legally end the input
would have an empty FOLLOW contribution and the parser couldn't tell that
finishing is allowed. `$` is a genuine element of the sets and a genuine
terminal the driver appends (Module 26).

---

## Computing it

### Least fixed point, same machinery as Module 21 — (a)
Rule 3 makes FOLLOW self-referential (the dangling-else grammar:
`FOLLOW(S)` needs `FOLLOW(X)` needs `FOLLOW(S)`). Start every set empty (except
the `$` seed), add only, iterate until a full pass changes nothing. Monotone +
bounded ⇒ terminates on the least fixed point.

### FIRST is a fixed input, computed first — (c)
`FollowSets` builds a `FirstSets` (Module 21) in its constructor and treats it
as constant. FOLLOW never feeds back into FIRST, so there's no reason to
interleave them; computing FIRST to completion first keeps the FOLLOW iteration
a single clean loop.

### `firstOfSeq(β)` handles the "β nullable" test — (c)
Rule 3 fires exactly when `ε ∈ FIRST(β)`. That's the same predicate as "β is
nullable", and `FIRST(β)` already carries it. One call gives both the tokens to
add (rule 2) and the nullable flag (rule 3).

### Every round snapshotted — (c)
Like Module 21: `rounds` keeps a deep copy per pass so `Main` prints the
convergence trace and tests assert on round counts.

---

## Cross-check

### Verified against Module 20's `Predict.followOf` — (c)
Module 20 computes FOLLOW inline as part of its LL(1) check; this module
computes it standalone on top of an explicit FIRST. Independent code paths,
same answer — printed as `MATCH`, asserted in the tests.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | The three canonical FOLLOW rules, unchanged | (b) | a "simpler" 2-rule version — drops rule 3, wrong |
| 2 | Seed `$` into `FOLLOW(start)` | (a) | special-case EOF in the driver only |
| 3 | Least fixed point by iteration | (a) | direct evaluation — impossible on the cycle |
| 4 | FIRST computed first, held constant | (c) | interleave FIRST and FOLLOW iterations |
| 5 | `firstOfSeq(β)` for both tokens and nullability | (c) | a separate `nullable(β)` call |
| 6 | Snapshot every round | (c) | keep only final sets |

---

## What We Proved

1. **FOLLOW flows through nullable tails.** `abc.grammar`: `FOLLOW(A) = {b, c}`
   — `b` from `B`, and `c` because `B` is nullable.

2. **The circular case converges.** `danglingelse.grammar`: `FOLLOW(S)` and
   `FOLLOW(X)` are mutually dependent and both resolve to `{else, $}`.

3. **Operators propagate up the primed chain.** `expr.grammar`: `FOLLOW(F)` ends
   up `{), *, +, $}` — every operator that can appear after a factor.

4. **`epsilon` is never in a FOLLOW set**, and **`$` is always in
   `FOLLOW(start)`**.

5. **It's a fixed point** (`one more pass changes nothing? true`) and **agrees
   with Module 20** on every nonterminal of every grammar.
