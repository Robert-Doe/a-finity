# Module 11 — Decisions

Choices in `Nfa.java` / `nfa.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## Representing the NFA

### `delta` maps to a **Set** of states, and is **not** completed to total — (a)
The two things that make an NFA an NFA: `delta(q, a)` can have zero, one, or many
targets, and "zero" (no transition) is a legal outcome — a path that dies. The
DFA loader (Module 10) added a `<dead>` state to fill holes; the NFA loader
deliberately does **not**. An empty transition set just means "this branch ends
here."

### `epsilon` is a symbol on transition lines, kept out of `alphabet` — (c)
`I epsilon U` is a valid line. But `alphabet` holds only real input symbols, so
`accepts` rejects any *input* symbol not in it, while `epsilon` transitions are
consulted only by `epsilonClosure`. Keeping the two separate stops "did the user
type an epsilon?" confusion.

### States and symbols are ordered strings — (c)
`LinkedHashSet` / ordered arrays, same as Module 10, for deterministic output.
`showSet` sorts (`TreeSet` / `.sort()`) so a state set prints the same
regardless of insertion order — important because the simulator builds sets in
data-dependent order.

---

## Simulation

### The simulator tracks a **set** of states — (a)
"Nondeterministic" doesn't mean "pick randomly" — it means "explore all
possibilities at once." `accepts` keeps `cur`, the set of every state the NFA
could be in, and updates it per symbol: `cur = epsilonClosure(union of
move(s, symbol) for s in cur)`. Accept iff the final set meets `F`. This is a
breadth-first exploration of all paths, done in lockstep — O(states × input),
no backtracking.

### `epsilonClosure` is a worklist reachability search — (a)
Given a set, repeatedly add any state reachable by one `epsilon` step, until
nothing new. It's `⇒*` (Module 4) restricted to `epsilon` edges — a
reflexive-transitive closure. Run it once on `{start}` before the first symbol
(so `epsilon`-reachable accepting states make `""` acceptable) and once after
every symbol.

### `accepts` bails early on an empty set — (c)
If `cur` becomes empty mid-input, every branch has died; no later symbol can
revive it, so return `false` immediately. Correctness doesn't require it (the
final intersection would be empty anyway) but it's the honest short-circuit.

---

## Epsilon elimination — the "no power" proof

### `removeEpsilon()` builds an equivalent NFA with no `epsilon` edges — (b)
The standard construction:
- **new `delta'(q, a)`** `= epsilonClosure( union of move(p, a) for p in epsilonClosure(q) )`.
  Take everything `q` can reach by `epsilon`, see where a real `a` leads, then
  take the `epsilon`-closure of *that*.
- **`q` is accepting** in the new NFA iff `epsilonClosure(q)` contains an old
  accepting state.
- **start stays the same.**
The result has the same states, no `epsilon` transitions, and — verified by
enumeration — exactly the same language. That is the proof that `epsilon` adds
no expressive power: anything an `epsilon`-NFA accepts, an `epsilon`-free NFA
accepts.

### We do NOT also remove the nondeterminism here — (c)
`removeEpsilon()` leaves an NFA (still multi-valued `delta`), just `epsilon`-free.
Flattening *that* to a DFA is the subset construction, Module 13. Splitting the
two removals — `epsilon` here, nondeterminism there — keeps each module's proof
to one idea.

### The `ab-star.nfa` fixture is a real Thompson gadget — (c)
It's built exactly as Module 12's construction will build `(a|b)*`: an inner
union with a shared entry `U` and shared exit `W`, wrapped in a star with
`I -eps-> U`, `I -eps-> F`, `W -eps-> U`, `W -eps-> F`. The first draft wired the
loop-back to one arm of the union and silently became `a* ∪ b*` — a good
cautionary tale, now in `run.md`.

---

## Cross-checks

### NFA(a|b)*abb vs DFA(Module 10) vs Regex(Module 3) — (c)
Three independent implementations of the same regular language, agreeing on
every string up to length 6–7, both directions where it makes sense. This is
the empirical version of "regular = DFA = NFA = regex"; the constructive proofs
are Modules 12–13.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | `delta` → set, partial allowed | (a) | total `delta` — that's a DFA |
| 2 | `epsilon` a symbol, not in `alphabet` | (c) | a separate `epsilonDelta` map — more surface |
| 3 | Simulate a set of states in lockstep | (a) | backtracking search — same result, exponential worst case |
| 4 | `epsilonClosure` = worklist reachability | (a) | recursive — same thing, stack-limited |
| 5 | Early-out on empty state set | (c) | run to the end — correct but wasteful |
| 6 | `removeEpsilon` keeps the NFA an NFA | (c) | go straight to a DFA — steals Module 13's job |
| 7 | Thompson-shaped `ab-star.nfa` | (c) | a minimal hand NFA — wouldn't preview Module 12 |

---

## What We Proved

1. **Nondeterminism works by exploring all paths at once.** The NFA for
   `(a|b)*abb` "guesses" where the final `abb` starts by taking `S -a-> S1`
   *and* `S -a-> S` on the same character. `trace("aabb")` shows the state set:
   `{S} → {S,S1} → {S,S1} → {S,S2} → {S,S3}` — and `S3` in the last set means
   accept.

2. **Its language is exactly the DFA's and the regex's.** 15 strings up to
   length 6, matched string-for-string against Module 10's DFA and Module 3's
   `(a|b)*abb`.

3. **`epsilon` transitions add no power.** `removeEpsilon()` on the `(a|b)*`
   `epsilon`-NFA produces an `epsilon`-free NFA that accepts the identical
   language (up to length 6), and it also equals regex `(a|b)*`. The
   `epsilon`-closure carried all the "free moves" into the real transitions.

4. **`epsilon`-closure is why `""` gets accepted.** `epsilonClosure({I})` for
   `(a|b)*` is `{A1, B1, F, I, U}` — it already contains the accepting `F`
   before a single character is read.
