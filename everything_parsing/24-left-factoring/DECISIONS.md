# Module 24 — Decisions

Choices in `LeftFactor.java` / `leftfactor.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The transform

### The standard rewrite — (b)
`A -> a β1 | ... | a βk | rest` becomes `A -> a A' | rest` and
`A' -> β1 | ... | βk`, with an empty `βi` producing `A' -> epsilon`. Dragon Book
§4.3.4. Language-preserving: every string `a βi …` is still generated, routed
through the stem `A'`.

### Factor the LONGEST shared prefix, not just the first symbol — (c)
`a b c | a b d | a e`: the longest prefix common to ≥ 2 alternatives is
`a b` (shared by the first two). We factor that, producing an intermediate
nonterminal, then a second pass factors `a` out of what remains. Factoring one
symbol at a time also terminates and is also correct, but longest-prefix keeps
the intermediate grammar closer to what a person would write and matches the
textbook worked examples.

### Iterate to a fixed point, restarting the pass after each change — (c)
`factor` loops over all nonterminals; the moment `factorOne` changes something
it `break`s and restarts. Simple and obviously correct (the grammar strictly
progresses toward "no shared first symbols"); the O(n²) restart cost is
irrelevant at this scale.

### Fresh names by apostrophe suffix — (c)
`A'`, then `A''` if taken — same convention as Module 23's left-recursion
elimination, so a grammar run through both transforms has consistent naming.

---

## What left factoring does and does not fix

### It makes alternatives FIRST-disjoint *where the strings actually diverge* — (a)
After factoring, no two alternatives of a rule share a first symbol, so
one-token lookahead suffices — **unless** the ambiguity is not about a shared
prefix at all. The dangling else is the example: after factoring,
`S' -> epsilon | else S` has disjoint FIRST sets (`{}` vs `{else}`), but
`else ∈ FOLLOW(S')`, so the nullable/FOLLOW conflict from Module 20 survives.
Left factoring is necessary, not sufficient, for LL(1).

### It changes parse-tree shape — (a)
Like left-recursion elimination, the extra stem nonterminal adds a node. The
concrete tree for `if b then S else S` now goes through `S'`. Module 39's AST
lowering drops these.

---

## The language check

### Verified empirically by bounded enumeration — (c)
Same approach and rationale as Module 23: enumerate every string up to a small
length bound from both grammars and assert set equality. Catches the real
mistakes (an alternative put in the wrong group, a missing `epsilon` suffix).

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Standard prefix-extraction rewrite | (b) | ad hoc grouping |
| 2 | Longest shared prefix per step | (c) | first symbol only — more intermediate nonterminals |
| 3 | Iterate to fixed point, restart on change | (c) | single pass — misses nested prefixes |
| 4 | Apostrophe-suffix fresh names | (c) | numbered names — inconsistent with Module 23 |
| 5 | Empirical language equivalence | (c) | trust the transform |
| 6 | Factor only prefixes of length ≥ 1 | (a) | length-0 "prefix" → infinite loop |

---

## What We Proved

1. **The dangling-else grammar is derived by factoring.**
   `S -> if b then S | if b then S else S | other` →
   `S -> other | if b then S S'`, `S' -> epsilon | else S` — the exact grammar
   Modules 20 and 22 used.

2. **Nested prefixes need multiple passes.** `a b c | a b d | a e` factors `b`
   first (longest), then `a`.

3. **A realistic case collapses.** Three `id`-prefixed statement forms become
   `stmt -> id stmt'`.

4. **The language is preserved** in every case (bounded enumeration), and an
   already-factored grammar is returned unchanged.

5. **Factoring is necessary but not sufficient for LL(1)** — the dangling-else
   `else` conflict is a FOLLOW-set issue that survives factoring.
