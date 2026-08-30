# Module 21 — Decisions

Choices in `FirstSets.java` / `firstsets.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## Why a fixed point at all

### The definitions are circular — you cannot read them off — (a)
`nullable(A)` for `A -> B C` depends on `nullable(B)` and `nullable(C)`, which
(in a grammar with cycles) can depend back on `nullable(A)`. Same for FIRST:
`FIRST(E)` for `E -> T Ep`, `T -> F Tp`, `F -> ( E )` depends on itself. There
is no evaluation order that resolves this by substitution. The standard answer
is a **least fixed point**, reached by iteration from the bottom.

### Start empty, add only, stop when a pass adds nothing — (a)
- **Monotone**: every rule can only *add* elements, never remove.
- **Bounded**: nullable is a subset of the finite nonterminal set; each FIRST
  set is a subset of the finite terminal set plus `{epsilon}`.
- Monotone + bounded ⇒ the iteration reaches a fixed point in finitely many
  rounds. Because we grew from empty and only added rule-forced elements, that
  fixed point is contained in every fixed point — it is the **least** one, which
  is the correct answer (a bigger set would assert derivations that don't
  exist).

### Round-robin over all productions, not a worklist — (c)
A worklist (re-examine only productions whose FIRST inputs changed) is faster on
big grammars. For teaching, a full pass per round makes the trace legible: you
can watch `FIRST(E)` stay empty in round 1, gain `{ ( id num }` in round 3, and
freeze. Speed is not the point here; Module 28's Project 2 tool can optimise.

---

## Representation

### `nullable` is a `Set` of nonterminal names; a sequence is nullable iff every symbol is in it — (c)
Terminals are never in the set, so `nullableSeq` is a simple "all members?"
check. An empty sequence is vacuously nullable — which is exactly what makes
`A -> epsilon` (empty rhs) put `A` in the set.

### `epsilon` rides inside FIRST as a sentinel string — (c)
Consistent with Module 20. `FIRST(seq)` contains `"epsilon"` iff `seq` is
nullable. One value carries both "possible first terminals" and "can vanish."

### Every round is snapshotted — (c)
`nullableRounds` and `firstRounds` keep a deep copy per round so `Main` can
print the convergence trace and the tests can assert on intermediate states
(e.g. "round 1 finds only `Z`"). Costs memory proportional to
rounds × grammar size; negligible at this scale and worth it for the visible
proof that the iteration converges.

---

## Cross-check

### `Main` and the tests verify agreement with Module 20 — (c)
Module 20's `Predict` computes FIRST by its own iteration. This module computes
it via an explicit `nullable` set first. They are independent code paths; the
output prints `MATCH` and the test suite asserts set equality per nonterminal.
A divergence would mean one of them is wrong.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Least fixed point by iteration from empty | (a) | substitution / evaluation order — impossible on cycles |
| 2 | Stop when a full pass adds nothing | (a) | fixed number of rounds — may under- or over-run |
| 3 | Round-robin, not a worklist | (c) | worklist — faster, less legible trace |
| 4 | `nullable` as a Set; empty seq vacuously nullable | (c) | a per-symbol boolean map |
| 5 | `epsilon` sentinel inside FIRST | (c) | separate nullable flag on every return |
| 6 | Snapshot every round | (c) | keep only the final tables |
| 7 | Cross-check vs Module 20 in output + tests | (c) | trust one implementation |

---

## What We Proved

1. **Nullable is a least fixed point.** `indirect.grammar`: round 0 `{}`,
   round 1 `{Z}`, round 2 `{Y,Z}`, round 3 `{X,Y,Z}`, round 4 unchanged.

2. **FIRST needs the iteration because it is circular.** `expr.grammar`:
   `FIRST(E)` is empty after rounds 1 and 2, resolves to `{ ( id num }` in
   round 3 once `T` and `F` have filled in.

3. **FIRST reaches past a nullable prefix.** `cascade.grammar`: `FIRST(S)` for
   `S -> A B C d` is `{a, b, c, d}` — every nullable alternative's FIRST plus
   the first non-nullable symbol — and `epsilon` is *not* in it because `S`
   isn't nullable.

4. **It is a fixed point.** `one more pass changes nothing? true` for all three
   grammars.

5. **Two independent implementations agree.** This module's FIRST equals
   Module 20's `Predict.firstOf` on every nonterminal of every grammar.
