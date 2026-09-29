# Module 12 — Decisions

Choices in `Thompson.java` / `thompson.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The construction

### One gadget per regex node, each with a single entry and single exit — (b)
This is Thompson's construction (1968), the standard. The invariant that makes
it compose: every fragment has exactly one `start` state and one `end` state, so
a parent gadget can wire to those two without knowing anything about the
fragment's internals.

### Concat adds **zero** states, via an ε-edge `l.end -> r.start` — (c)
Thompson's original paper *merges* `l.end` with `r.start` (also zero new
states). The ε-edge variant keeps them distinct, which makes the code uniform
(every fragment keeps its own two endpoints) at the cost of one ε-transition
that `removeEpsilon` will collapse anyway. Either way the state budget is the
same.

### Union and Star each add exactly 2 states; leaves cost 2 — (a)
Forced by the shape of each gadget: Union needs a fresh branch-in and a fresh
join-out; Star needs a fresh loop-entry and a fresh loop-exit; a Char needs a
"before" and an "after." This is what makes the bound `#states ≤ 2·#nodes`
provable by structural induction — and, since `#nodes ≤ |regex|` (counting the
regex without parentheses), `#states ≤ 2·|regex|`.

### Fresh state names are `q0, q1, q2, …` from one counter — (c)
A single incrementing counter across the whole tree. The counter's final value
*is* the state count, so `stateCount(re)` runs `frag` and reads the counter
without building the text — useful for the bound table.

### `∅` (Empty) builds 2 states and **no transitions** — (a)
A fragment with an unreachable accept state accepts nothing — exactly `L(∅)`.
`ε` (Epsilon) builds `s -ε-> e`, so it accepts only the empty string. These are
the base cases the induction rests on.

---

## Building the NFA

### Thompson emits `.nfa` text and calls `Nfa.parse` — (c)
Same pattern as Module 8 (EBNF → BNF text → `Grammar.parse`). `Nfa`'s
constructor is private; rather than widen its API, the construction produces the
transition list as text and reuses the tested loader. One format, one parser.

### `Nfa.parse` here allows an **empty alphabet** — (a), a small relaxation
An NFA over `Σ = ∅` is legal — it accepts at most the empty string. Thompson can
produce one (from `∅` or `ε`), so this module's copy of `Nfa.parse` drops the
"alphabet must be non-empty" check. Module 11's copy keeps it, since all its
fixtures have alphabets. The two copies diverge by this one line; noted here.

### The alphabet is collected from the regex's `Char` nodes — (a)
The only real input symbols a regex mentions. Gathered by a walk, sorted for
deterministic output.

---

## Verification

### Three-way cross-check: Thompson NFA vs regex vs Module-10 DFA — (c)
`L(Thompson NFA)` up to length 6–7 is checked against `Regex.matches` (Module 3)
and against the hand-written DFA (Module 10), both directions where sensible.
This is the empirical confirmation that Thompson's construction is
language-preserving; the structural-induction proof is standard and sketched in
the tutorial.

### Also run `removeEpsilon` on the result — (c)
Shows the full front half of the pipeline: `regex → ε-NFA (Thompson) → ε-free
NFA (Module 11)`. The language is unchanged at every step. Module 13 adds the
last stage (→ DFA).

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | One gadget per node, single entry/exit | (b) | a non-compositional construction — can't induct on it |
| 2 | Concat via ε-edge, +0 states | (c) | merge `l.end`/`r.start` — same budget, less uniform code |
| 3 | Fresh names from one counter | (c) | per-fragment renaming — counter doubles as the size |
| 4 | `∅` = 2 states, 0 transitions | (a) | a special "reject" state — unnecessary, the shape already does it |
| 5 | Emit text, reuse `Nfa.parse` | (c) | a public `Nfa` builder — new surface |
| 6 | Allow empty alphabet in this `Nfa.parse` | (a) | forbid `∅`/`ε`-only regexes — arbitrary limit |
| 7 | Three-way cross-check | (c) | trust the textbook — this course verifies |

---

## What We Proved

1. **Every regex node compiles to a fixed-size NFA gadget.** `a(b|c)` →
   8 states, 8 transitions, wired exactly as the gadget rules say.

2. **The state count never exceeds `2·nodes`** — and, without parentheses,
   `2·|regex|`. The bound table checks 10 regexes; `(a|b)*abb` hits it exactly
   (14 states = 2 × 7 characters).

3. **The construction preserves the language.** `L(Thompson NFA for (a|b)*abb)`
   up to length 6 is the same 15 strings as `Regex.parse("(a|b)*abb")` and as
   Module 10's hand-written DFA.

4. **`regex → ε-NFA → ε-free NFA` works end to end.** Thompson builds a 14-state
   ε-NFA; `removeEpsilon` (Module 11) yields a 14-state ε-free NFA; the language
   is identical throughout. Module 13 finishes the job with `→ DFA`.
