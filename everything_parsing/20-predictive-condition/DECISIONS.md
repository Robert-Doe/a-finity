# Module 20 — Decisions

Choices in `Predict.java` / `predict.mjs`, in three buckets: **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The condition

### PREDICT, not just FIRST — (a)
A predictive parser picks `A -> a` when the current token could begin what `a`
expands to. If `a` is nullable, "what `a` expands to" also includes *whatever
can follow `A`*, because taking `a` might consume nothing. So the decision set
is `FIRST(a)` for non-nullable `a`, and `FIRST(a)\{epsilon} U FOLLOW(A)` for
nullable `a`. Checking only FIRST would call the dangling-else grammar LL(1),
which it is not.

### LL(1) = pairwise-disjoint PREDICT sets, per nonterminal — (b)
This is the definition (Aho/Sethi/Ullman, "Dragon Book" §4.4.3; CSE 340). We
report every overlapping *pair* with the *token* that overlaps — a concrete
non-LL(1) witness, not just a yes/no. That's what makes the output useful for
grammar debugging (and it's Project 2's job, Module 28).

---

## Computing FIRST / FOLLOW

### Iterate the standard rules to a fixed point — (c), with a forward pointer
FIRST: start every nonterminal empty, repeatedly fold each production's
`FIRST(rhs)` into `FIRST(lhs)`, stop when a full pass adds nothing. FOLLOW:
seed `FOLLOW(start) = {$}`, then for each `B -> a X b` add `FIRST(b)\{epsilon}`
to `FOLLOW(X)` and, if `b` is nullable, add `FOLLOW(B)` too; iterate to
convergence.

This *is* the least-fixed-point algorithm. Module 20 uses it as a tool;
**Modules 21 and 22 are where it gets justified** — why the iteration
terminates (the sets only grow, bounded by the finite terminal alphabet), why
it lands on the *least* fixed point, and how `nullable` and the `$` marker
behave in the corner cases. Splitting it this way keeps Module 20 about the
*condition* and lets 21–22 be about the *computation*.

### `epsilon` travels inside FIRST as a marker — (c)
`FIRST(seq)` contains the literal string `"epsilon"` iff every symbol in `seq`
is nullable. One representation for "these are the possible first terminals"
and "the whole thing might vanish," rather than a separate boolean returned
alongside. `nullable(seq)` is then just `FIRST(seq).contains("epsilon")`.

### `$` is a real element of FOLLOW sets — (b)
The end-of-input marker is written `$` and lives in FOLLOW sets like any
terminal. A nullable production whose nonterminal can end the input predicts on
`$`, and the parser's driver supplies a synthetic `$` token at EOF (Module 26).

### Left-recursive grammars: FIRST under-approximates, and that's acceptable here — (c)
For `A -> A x | y` the fixed-point FIRST is `{y}` — correct, as it happens. In
general a left-recursive rule can make this iteration miss nothing (FIRST is
still exact) but the grammar is **never LL(1)** anyway: `A -> A a` and
`A -> b` always share `FIRST` on `b`. The conflict check catches it. We don't
special-case left recursion; Module 23 removes it.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Decision set is PREDICT (FIRST + FOLLOW-if-nullable) | (a) | FIRST only — misses nullable conflicts |
| 2 | Report the conflicting (nonterminal, token, pair) | (b) | a bare LL(1) yes/no |
| 3 | Fixed-point iteration for FIRST/FOLLOW | (c) | memoised recursion — subtle on cycles; deferred rigor to M21/22 |
| 4 | `epsilon` as a member of FIRST | (c) | separate nullable flag threaded everywhere |
| 5 | `$` a normal FOLLOW element | (b) | special-case EOF in the driver only |
| 6 | No special handling of left recursion | (c) | detect + reject early — the conflict check already does |

---

## What We Proved

1. **The Module 19 grammar is LL(1).** Every nonterminal's PREDICT sets are
   disjoint — which is *why* recursive descent could choose alternatives by a
   single token in Module 19.

2. **A nullable list rule can still be LL(1).** `L -> S L | epsilon` with
   `FIRST(S L) = {id}` and `FOLLOW(L) = {$}` — disjoint.

3. **A shared prefix breaks LL(1) on a concrete token.** `S -> a b c | a b d`
   → conflict on `a`; the fix is left factoring (Module 24).

4. **The dangling else is a nullable/FOLLOW conflict.** `FOLLOW(X)` contains
   `else` because `S X` sits inside `S -> if b then S X`, so `X -> else S` and
   `X -> epsilon` both predict `else`.

5. **Left recursion is never LL(1)** — the conflict check flags `A -> A x | y`
   on `y`.
