# Module 13 — Decisions

Choices in `Subset.java` / `subset.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The construction

### A DFA state *is* a set of NFA states — (a)
This is the powerset construction. The DFA simulates the NFA's "which states
could a clone be in" set (Module 11) by making that set a single state. The
three rules — start = `εClosure({q0})`, `δ(S,a) = εClosure(⋃ move(s,a))`,
accepting iff `S ∩ F ≠ ∅` — are forced by that identification.

### Only reachable subsets are built — (a)
A worklist: start from the start subset, and for each subset and symbol, compute
the target subset; if it's new, enqueue it. There are `2^|Q|` subsets in
principle, but most are unreachable — the Thompson NFA for `(a|b)*abb` (14
states, `2^14` = 16384 subsets) determinizes to **5**. The worst case is still
`2^k`, which `kthFromEndNfa` hits exactly.

### Subsets are keyed by their sorted contents — (b)
`{q3, q1, q7}` and `{q1, q7, q3}` are the same DFA state. The key is the sorted
join, so the map recognises them as one. Getting this wrong (keying by insertion
order) would split one DFA state into several and inflate the count.

### DFA states are named `D0, D1, …` with a separate legend — (c)
Naming a DFA state `{q0,q2,q4,q6,q7,q8}` makes the transition table unreadable.
`D0…Dn` in discovery order, plus a `legend()` mapping each name back to its
set. `D0` is always the start.

### `Subset` emits `.dfa` text and calls `Dfa.parse` — (c)
Same pattern as Modules 8 and 12: build the artifact as text, reuse the tested
loader (Module 10's `Dfa`). The resulting DFA is a first-class Module-10 `Dfa`,
so all its machinery — `trace`, `language`, `acceptsString` — works unchanged.

### `Dfa.parse` / `Nfa.parse` here allow an **empty alphabet** — (a)
`Thompson.build(∅)` produces an NFA with no `Char` nodes, hence `Σ = ∅`.
Determinizing it gives a one-state DFA that accepts nothing. Both loaders in
this module drop the "alphabet must be non-empty" guard, and `rest()` returns
`[]` (not `[""]`) for an empty `alphabet:` line — the bug that first crashed
`SubsetTest`.

---

## The blow-up example

### `kthFromEndNfa(k)`: "the k-th symbol from the end is `a`" — (b)
The canonical NFA/DFA size-gap example. The NFA has `k+1` states: `s0` loops on
everything and *also* guesses (`s0 -a-> s1`) that the current `a` is the one `k`
from the end; `s1…sk` then count down `k-1` more symbols. Its minimal DFA has
`2^k` states because a deterministic machine must *remember the last `k`
symbols* — one bit per position — and there are `2^k` such memories.

### Verified for `k = 1…6` — (c)
`stateCount(DFA) == 2^k` exactly, every time, plus a spot-check that the DFA
decides `baab` (3rd-from-end `a`) and `abaa` (3rd-from-end `b`) correctly.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | DFA state = set of NFA states | (a) | there is no other powerset construction |
| 2 | Build reachable subsets only | (a) | build all `2^n` — wasteful and usually impossible |
| 3 | Key subsets by sorted contents | (b) | key by object identity / insertion order — splits states |
| 4 | Name `D0…Dn`, keep a legend | (c) | name by set contents — unreadable table |
| 5 | Emit text, reuse `Dfa.parse` | (c) | a public `Dfa` builder — new surface |
| 6 | Allow empty alphabet; `rest() → []` | (a) | forbid `∅`-regex — arbitrary |
| 7 | `kthFromEndNfa` as the blow-up witness | (b) | a contrived NFA — this one is standard and easy to check |

---

## What We Proved

1. **Every NFA has an equivalent DFA, built mechanically.** The Thompson
   ε-NFA for `(a|b)*abb` (14 states) determinizes to a 5-state DFA whose
   language — 31 strings up to length 7 — is identical to the NFA's, to the
   Module-3 regex's, and to Module 10's hand-written DFA's.

2. **A DFA state is literally "everywhere a clone could be."** The legend shows
   `D0 = εClosure({start})`, and every `Dn` is the set of NFA states reachable
   on that input prefix. An accepting `Dn` is exactly one whose set contains an
   NFA accepting state.

3. **The blow-up is real and exactly `2^k`.** The `k`-th-from-end NFA has
   `k+1` states; its DFA has `2, 4, 8, 16, 32, 64` for `k = 1…6` — no slack.
   This is why lexer generators sometimes keep the NFA and simulate it, or
   determinize *lazily* (build DFA states on demand).

4. **The pipeline `regex → NFA → DFA` is complete.** Module 12 built the first
   arrow, this module the second. What remains is making the DFA *smallest* —
   Module 14.
