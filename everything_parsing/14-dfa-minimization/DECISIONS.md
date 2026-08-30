# Module 14 — Decisions

Choices in `Minimize.java` / `minimize.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The theory

### Equivalent states = same right language — (a)
Two states `p, q` are equivalent iff, for *every* string `w`, running from `p`
accepts `w` exactly when running from `q` does. Merge each equivalence class and
you get a DFA with the fewest possible states. The **Myhill-Nerode theorem**
says this minimal DFA is *unique* up to renaming, and its state count equals the
number of distinct right languages of the language itself.

### `trim` first: unreachable states don't count — (a)
A state you can never get to from the start contributes nothing to the language.
Both algorithms remove unreachable states before partitioning (BFS from the
start). The subset construction (Module 13) already produces no unreachable
states, but a hand-written DFA might have some.

---

## Two algorithms, deliberately

### partitionRefinement — Moore's method, top-down — (b)
Start with the coarsest partition that could possibly be right:
`{accepting} | {non-accepting}`. Each round, compute every state's *signature* —
(its current block, then the block each symbol sends it to) — and split any
block whose members have different signatures. Stop when a round produces no new
blocks. This is Moore's algorithm; Hopcroft's is a cleverer near-linear variant,
but Moore is the one you can trace by hand.

### tableFilling — bottom-up, all pairs — (b)
The dual view. Mark pairs known to be *distinguishable*: first the pairs where
one is accepting and one isn't, then — repeatedly — any pair `(p,q)` such that
some symbol sends it to an already-marked pair. When it stabilises, the unmarked
pairs are exactly the equivalent ones; union-find groups them into classes.

### Both, and check they agree — (c)
They compute the *same* partition (a theorem), so `Main` runs both and asserts
the classes match. It's a strong internal consistency check, and the two
framings — "keep splitting" vs "keep marking distinguishable" — genuinely help
different people.

### `minimal()` uses partition refinement — (c)
Either would do; refinement's output (blocks in first-encounter order) is a
touch more convenient to name deterministically.

---

## Building the quotient DFA

### States named `M0…Mn`, start's class first — (c)
Order the classes so the one containing the start state is `M0`, then by
smallest member name. Deterministic, and the start is always `M0`. Each class's
transitions are read off any representative (they all agree, by construction).

### A class is accepting iff its representative is — (a)
The initial partition separates accepting from non-accepting, and no later split
merges across that line, so every member of a class has the same
accepting-status. Checking the representative is enough.

### Emit `.dfa` text, reuse `Dfa.parse` — (c)
Same pattern as Modules 8, 12, 13. The minimal DFA is a first-class Module-10
`Dfa`.

---

## Regex equivalence

### `equivalent(a, b)` = isomorphism of minimal DFAs — (a)
`L(a) = L(b)` iff `minDFA(a)` and `minDFA(b)` are isomorphic. Because the minimal
DFA is *unique*, isomorphism is the exact test — not just "same state count."
`isomorphic()` walks both DFAs in lockstep from their start states, pairing
states; any mismatch in accepting-status or in where a paired pair's transitions
land means not isomorphic.

### This is *decidable* — the point of the module — (a)
Regular-language equivalence is decidable, and here is the procedure:
regex → NFA → DFA → minimal DFA → compare. Contrast context-free languages,
where equivalence is *undecidable* (Module 6) — which is why Part IV's grammars
can't be "equivalence-checked" and must instead be forced into an LL/LR shape.

### Regexes must not contain spaces — (b), a Module-3 constraint
Module 3's regex parser has no whitespace-skipping, so a space becomes a
`Char(' ')`. `(ab)*ab | ()` would compile a space-matching automaton. All
regexes in this module are written without spaces.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Equivalence = same right language | (a) | there is no other notion |
| 2 | `trim` unreachable states first | (a) | leave them — inflates the count, wrong answer |
| 3 | Ship both refinement and table-filling | (c) | one only — the cross-check is valuable and the two views help |
| 4 | Moore's refinement, not Hopcroft's | (c) | Hopcroft — faster, much harder to read |
| 5 | Name classes `M0…`, start first | (c) | arbitrary names — non-deterministic output |
| 6 | Regex equivalence = minimal-DFA isomorphism | (a) | compare bounded enumerations (Module 6) — evidence, not proof |

---

## What We Proved

1. **Every DFA has a unique minimal equivalent.** The subset-construction DFA
   for `(a|b)*abb` has 5 states; two of them (`D0`, `D2` — both meaning "0
   characters of `abb` matched so far") are equivalent, so the minimal DFA has
   **4**. Partition refinement and table filling agree on exactly this.

2. **The minimal DFA is the "right" one.** It is isomorphic to the DFA a human
   wrote by hand in Module 10 — and that hand-written DFA turns out to have
   already been minimal.

3. **Myhill-Nerode, made concrete.** `L((a|b)*abb)` has exactly 4 right-language
   classes ("matched 0 / 1 / 2 / 3 characters of the suffix `abb`"), so its
   minimal DFA has exactly 4 states — no fewer is possible.

4. **Regular-language equivalence is decidable.** `(a|b)* == (a*b*)*` (both
   `Σ*`), `a** == a*`, `a(b|c) == ab|ac` — all `true`, each proved by building
   and comparing minimal DFAs. `a* != a+`, `ab|ba != (a|b)(a|b)` — `false`,
   with the distinguishing string implicitly found. Context-free equivalence
   has no such procedure.
