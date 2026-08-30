# Module 28 — Decisions  (CSE 340 Project 2)

Choices in `GrammarTool.java` / `grammartool.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## This module writes no new algorithm

### It is pure integration — (c)
Every section is a component already built and tested in Modules 20–27:
`FirstSets` (21), `FollowSets` (22), `LeftRec` (23), `LeftFactor` (24),
`Predict` (20), `LL1Table` (25). `GrammarTool` composes them and formats one
report. That is the point of a "project" module — prove the pieces fit.

### Section order follows the CSE 340 project — (b)
Symbols → nullable → FIRST → FOLLOW is the Bazzi Project 2 task order. We append
the two grammar transforms and the LL(1) verdict, which the course covers in the
same unit.

---

## Composition choices

### Transformed grammars are serialised back to text and re-parsed — (c)
`LeftRec.paull` and `LeftFactor.factor` return a mutable `LeftRec.G`.
`toGrammar` renders it (`%start X` + productions) and runs it back through
`Grammar.parse`, so section 7's "does the fix work?" check uses the *same*
`Predict` / `Grammar` pipeline as everything else — no special path for
transformed grammars. Requires the loader to accept `'` in nonterminal names,
which it does.

### The fix order is: remove left recursion, THEN left-factor — (a)
Left-recursion elimination (Paull) can *introduce* shared prefixes (its
substitution step), so factoring must come second. The reverse order can leave
left recursion in place. Section 7's combined check does
`factor(paull(g))`.

### Paull's substitution is guarded (carried over from Module 23) — (c)
Only substitute `Aj` into `Ai -> Aj γ` when `Aj` can reach `Ai` as a leftmost
symbol. Without the guard, JSON's `elements -> value comma elements` gets
`value` inlined 16 ways for no benefit. With it, section 5 leaves
non-left-recursive rules alone.

---

## Reporting

### Fixed-width columns, sorted sets — (c)
`FIRST(%-9s)` / `FOLLOW(%-9s)` padding, `epsilon` sorted last in FIRST, `$`
sorted last in FOLLOW — consistent with Modules 21–22 so a reader who has seen
those recognises the format.

### Section 7 shows the table only when LL(1); shows conflicts + the fix otherwise — (c)
A conflicted grammar has no useful table (cells are double-booked), so printing
one would mislead. Instead: list the conflicts, then answer the question the
reader actually has — "can this grammar be made LL(1)?" — by running the
transforms and re-checking.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Pure composition, no new code | (c) | re-implement pieces for a "cleaner" tool |
| 2 | CSE 340 section order | (b) | arbitrary order |
| 3 | Serialise + re-parse transformed grammars | (c) | a separate analysis path for `LeftRec.G` |
| 4 | Remove recursion, then factor | (a) | factor first — can leave left recursion |
| 5 | Guarded Paull substitution | (c) | textbook Paull — bloats non-recursive rules |
| 6 | Table only when LL(1) | (c) | always print a (meaningless) table |

---

## What We Proved

1. **The whole LL(1) pipeline composes.** One program takes any CFG to a
   complete analysis: symbols, nullable, FIRST, FOLLOW, both transforms, verdict.

2. **An LL(1) grammar passes every section cleanly.** `stmts.grammar` — nullable
   `{L, P}`, disjoint predict sets, a single-valued table.

3. **A left-recursive grammar is diagnosed and fixed.** `expr.grammar` — 6
   conflicts; section 5 yields the Module 19 grammar; the transformed grammar
   is LL(1).

4. **A shared-prefix grammar is diagnosed and fixed.** `json.grammar` — no left
   recursion, but 10 conflicts; left factoring clears them; section 5 does
   *not* mangle the non-left-recursive list rules.

5. **The fix order matters and the tool gets it right** — recursion removal
   before factoring, verified by the "after ... LL(1) = YES" line.
