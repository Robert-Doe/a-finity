# Module 29 — Decisions

Choices in `FirstK.java` / `Backtrack.java` and mirrors, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## FIRST_k

### A k-prefix is a list of up to k terminals — (a)
FIRST_k(α) is the set of terminal strings of length ≤ k that can begin a
derivation from α — shorter than k only when α derives something that short.
FIRST_1 is the ordinary FIRST set (each element a singleton list). Stored as a
space-joined string in JS, a `List<String>` in Java.

### Fixed-point computation, same shape as FIRST — (a)
FIRST_k is circular (FIRST_k(E) via `F -> ( E )` depends on FIRST_k(E)). Start
every nonterminal's set empty, fold `FIRST_k(rhs)` in for each production,
iterate to convergence. An empty (not-yet-computed) nonterminal set contributes
**nothing** — a genuinely nullable nonterminal carries the empty string `""`
*inside* its set, so the join loop handles it without a special case. (Getting
this wrong pollutes FIRST_1 with phantom tokens.)

### k=1 is delegated to Module 20's exact PREDICT check — (c)
`minLL` returns 1 iff `Predict.isLL1()`. The FIRST_k test in this module ignores
the nullable-alternative case (which needs FOLLOW_k); for k=1 the exact check
already exists and handles it, so we use it. For k ≥ 2 the fixtures have no
nullable conflicted alternatives, so the FIRST_k-disjoint test is sound for
them — and `isLLk` bails out (returns false) if it meets a nullable alternative,
rather than giving a wrong answer.

### `minLL` searches k = 1..4 — (c)
Real LL(k) parsers rarely go past k=2; k=3 and k=4 are there to show "still no"
for the ambiguous grammar. Beyond 4 the FIRST_k sets explode and the point is
made.

---

## Backtracking recognizer

### It COUNTS parse trees, not just recognizes — (c)
`deriveSym` returns, per end position, the *number of ways* to derive the span.
The count at `position == input.length` is the number of parse trees. This makes
the ambiguity of `equiv.grammar` a concrete number (2ⁿ) rather than a vague
"it's ambiguous", and it makes the exponential blow-up measurable.

### `entries` counts production attempts — the work metric — (c)
Every time `deriveSym` tries one production of a nonterminal, `entries++`. No
memoization: the same `(nonterminal, position)` sub-problem is re-solved on
every path that reaches it, which is what makes an overlapping grammar
exponential. Packrat (Appendix X2) memoizes exactly that pair.

### Capped, and left-recursion is the caller's problem — (a)
`entries` is capped (default 20M); over the cap → `capped = true`,
`parseCount = -1`. Left-recursive grammars recurse forever — the caller checks
(`LeftRec.hasLeftRecursion`) or accepts the stack overflow. The fixtures are
all left-recursion-free.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | k-prefix = list of ≤ k terminals | (a) | a single "kth token" — loses the shorter cases |
| 2 | FIRST_k by fixed-point iteration | (a) | recursion — same circularity problem |
| 3 | Empty symSet contributes nothing | (a) | treat it as nullable — phantom FIRST tokens |
| 4 | k=1 delegated to exact PREDICT | (c) | FIRST_1-disjoint test — misses nullable cases |
| 5 | Recognizer counts parse trees | (c) | boolean recognize — can't show 2ⁿ |
| 6 | No memoization, count `entries` | (c) | memoize — hides the blow-up this module exists to show |
| 7 | Cap `entries`, report "capped" | (a) | run forever on a big exponential input |

---

## What We Proved

1. **One token is sometimes not enough.** `label.grammar`: both `stmt`
   alternatives have FIRST_1 = `{id}`, but FIRST_2 splits them
   (`{id colon}` vs `{id lparen, id semi}`) — it is LL(2), not LL(1).

2. **`label` can't be left-factored away.** `label` and `expr` are different
   nonterminals; the shared `id` isn't a literal prefix of one rule.

3. **No finite k helps an ambiguous grammar.** `equiv.grammar`: `A -> x B` and
   `A -> x C` have identical FIRST_k for every k, because `B` and `C` both
   derive `A`. `minLL` = none.

4. **Backtracking without memoization is exponential.** `equiv` on `x^n y`:
   2ⁿ parse trees, ~2ⁿ⁺¹ production tries.

5. **Disjoint FIRST sets keep it linear.** `expr` on `id (+ id)ⁿ`: one parse
   tree, production tries grow linearly.
