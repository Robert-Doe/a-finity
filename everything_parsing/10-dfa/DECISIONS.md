# Module 10 — Decisions

Choices in `Dfa.java` / `dfa.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## Representing the DFA

### `delta` as `Map<from, Map<symbol, to>>`, total by construction — (a)
The formal definition says `delta : Q x Sigma -> Q` is a **total** function.
The loader enforces it: after reading the file, any `(state, symbol)` pair with
no transition is routed to an implicit `<dead>` state, and `<dead>` self-loops
on every symbol. So `step()` never returns "undefined" — it returns a real
state, always. A partial transition table is an NFA in disguise (Module 11);
keeping this one total is what makes it a DFA.

### States and symbols are strings, in insertion order — (c)
`LinkedHashSet` / ordered arrays so the transition table and the language
enumeration print deterministically and the Java/JS builds match. Numeric state
IDs would be faster but less readable in a teaching artifact.

### The dead state is `<dead>`, added only if needed — (c)
The angle brackets keep it from colliding with a real state name. It's not
added if the file's transitions already cover every pair (like `even-a.dfa`).
`a-star-b-star.dfa` names its dead state `DEAD` explicitly — that's fine, it's
just a state with self-loops; the auto-added one is only for *missing*
transitions.

---

## Simulation

### `accepts` is a single left-to-right pass — (a)
Start at `q0`, one `step()` per input symbol, check membership in `F`. O(n), no
backtracking, no stack. This *is* the defining property: a DFA decides
membership in linear time with constant memory. A symbol outside `Sigma` fails
immediately.

### `language(maxLen)` brute-forces every string ≤ maxLen — (c)
It enumerates all `|Sigma|^0 + … + |Sigma|^maxLen` strings and keeps the
accepted ones. The first version tried to be clever (BFS over `(state, length)`
pairs with a visited set) and **was wrong** — it pruned distinct strings that
reached the same state at the same length, so it missed `babb`, `ababb`, etc.
For the small bounds this module uses (≤ 6), brute force is correct and fast.
Sort key: real string length (with `epsilon` counting as 0), then lexicographic.

### `repeatOn(symbol)` is the pumping lemma, made concrete — (c)
Run the DFA on `symbol` repeated. By pigeonhole, within `|Q| + 1` steps two
prefixes land on the same state. `repeatOn` returns that first `(i, j)` pair.
For `a*b*` it's `(0, 1)` — state `A` loops on `a` immediately, so the machine
literally cannot tell `a` from `aa` from `aaa`. That's *why* no DFA accepts
`{ a^n b^n }`, shown as a fact about a specific machine rather than a proof
sketch.

---

## The cross-check with Module 3's regex

### Both directions, up to a bound — (c)
Every string the DFA accepts (≤ 6) is fed to `Regex.matches`; every string the
regex matches (≤ 6) is fed to `Dfa.acceptsString`. Both must agree. This is
*evidence* that the DFA for `ends-abb` recognises exactly `(a|b)*abb` — the
exact equivalence is Module 13 (build the DFA from the regex) and Module 14
(minimal-DFA comparison), both decidable for regular languages.

### `ends-abb.dfa` is a suffix-tracking DFA — (c)
Its states `S / A / AB / ABB` mean "the longest suffix of the input seen so far
that is a prefix of `abb`." This is the hand-construction every string-search
algorithm (KMP, Aho–Corasick) generalises, and it's small enough to verify by
eye.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | `delta` total; auto-add `<dead>` | (a) | partial table — that's an NFA, not a DFA |
| 2 | String states, insertion order | (c) | integer IDs — faster, less readable |
| 3 | `<dead>` added only when needed | (c) | always add it — clutters `even-a`'s table |
| 4 | `language` = brute-force all strings ≤ maxLen | (c) | BFS with `(state,length)` visited set — the bug that shipped first |
| 5 | `repeatOn` returns the pigeonhole pair | (c) | just assert "DFAs can't count" — this shows it on a real machine |
| 6 | Cross-check both directions vs regex | (c) | one direction — misses a DFA that accepts *extra* strings |

---

## What We Proved

1. **A DFA is a total function plus a start and an accepting set**, and its
   `accepts` is one linear pass. `ends-abb.dfa` correctly accepts every string
   ending in `abb` and rejects the rest, verified by trace.

2. **A DFA's language is regular — and matches a regex exactly (on a bounded
   sample).** `ends-abb.dfa`'s language up to length 6 is identical, string for
   string, to `Regex.parse("(a|b)*abb")`'s — 15 strings, both directions
   checked.

3. **A DFA cannot count.** `repeatOn('a')` on the `a*b*` machine returns
   `(0, 1)`: the same state after zero `a`s and after one. It has no memory of
   how many it saw. `a*b*` — which it *does* accept — is the closest regular
   over-approximation of `{ a^n b^n }`, and it wrongly accepts `aaab`, `abbb`,
   `aabb`. Module 7 proved this must be so; here it is, on a four-line machine.
