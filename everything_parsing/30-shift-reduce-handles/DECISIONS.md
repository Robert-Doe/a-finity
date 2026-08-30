# Module 30 — Decisions

Choices in `ShiftReduce.java` / `shiftreduce.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The model

### Stack of grammar symbols + two moves — (b)
Shift-reduce parsing, Dragon Book §4.5. SHIFT pushes the next input terminal;
REDUCE `A -> b` requires `b` to be the top `|b|` stack symbols, pops them, pushes
`A`. Accept when the stack is `[start]` and the input is consumed.

### A handle is a (production, top-of-stack) pair — (b)
The handle of a right-sentential form is the substring that the *last* step of a
rightmost derivation expanded — and in a shift-reduce parse it is always at the
top of the stack when you reduce. We print it as `handle: b` next to each
reduce.

### Reductions reversed = a rightmost derivation — (a)
Each reduce undoes one rightmost-derivation step. Reverse the whole reduction
list and you get the derivation top-down. `rightmostDerivation` replays it,
always expanding the **rightmost** occurrence of the LHS, and the result must
equal the input — an executable check of the theorem.

---

## Finding handles without a table

### Bounded DFS: try reductions (in production order), then shift; backtrack — (c)
This module predates the LR automaton (Module 31). At each state we try every
reduction whose RHS is a stack suffix, then shifting, and backtrack from dead
ends. "Reduce first" plus backtracking finds the canonical parse for an
unambiguous grammar without needing lookahead logic. A `nodes` cap (2M) stops
runaway search; the fixtures finish far under it.

### This is a teaching device, not how LR parsers work — (c)
A real LR parser never backtracks — the DFA over viable prefixes (Module 31)
tells it shift-or-reduce in O(1) per token. We say so in the output and the
tutorial. The DFS here just makes the *mechanics* concrete first.

---

## Conflict detection

### A conflict = a reachable state with two moves that both reach ACCEPT — (c)
`firstConflict` walks the state space; at each state it counts how many distinct
moves (each reduce, and the shift) lead to an accepting configuration. Two or
more → a shift-reduce or reduce-reduce conflict, reported with the stack and
lookahead. For the ambiguous `E -> E + E` grammar this fires at `[E + E]` on
`+`. Budgeted to stay bounded.

### We report the first conflict, not all — (c)
One concrete witness is enough to show the grammar is ambiguous / not
shift-reduce parseable as written. Module 35 covers resolving them with
precedence.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Stack of symbols + SHIFT/REDUCE | (b) | anything else isn't shift-reduce parsing |
| 2 | Handle = top-of-stack RHS match | (b) | search the whole stack — the handle is always on top |
| 3 | Reductions reversed = rightmost derivation, checked | (a) | assert the theorem in prose |
| 4 | Bounded reduce-first DFS to find handles | (c) | build the LR automaton now (Module 31) |
| 5 | Conflict = ≥2 accepting moves from a state | (c) | only check the parse we found |
| 6 | Report the first conflict | (c) | enumerate all |

---

## What We Proved

1. **Bottom-up parses left recursion directly.** `E -> E + T | T` etc. is
   parsed as written — no elimination, no factoring.

2. **Every reduce is a handle, and reversing them is a rightmost derivation.**
   For `id + id * id` the reduction list reversed reproduces the input via a
   valid rightmost derivation (`final form == input? true`).

3. **Precedence appears in reduction order.** `T -> T * F` is reduced before
   `E -> E + T`.

4. **The stack is always a viable prefix.** Every STACK column is a prefix of
   some right-sentential form up to a handle end.

5. **An ambiguous grammar has a state with two handles.** `E -> E + E` on
   `id + id + id` reaches `[E + E]` where reduce and shift both lead to ACCEPT —
   a shift-reduce conflict.
