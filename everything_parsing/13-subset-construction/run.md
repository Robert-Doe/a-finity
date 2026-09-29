# Module 13 — Run It

From `13-subset-construction/`. Verified: OpenJDK 24, Node v22.14.0. No fixture arg.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
regex /(a|b)*abb/  ->  Thompson NFA (14 states, epsilon: true)  ->  subset-construction DFA (5 states)

each DFA state is the SET of NFA states a run could be in:
  D0   = {q0,q2,q4,q6,q7,q8}
  D1   = {q0,q1,q10,q2,q4,q5,q7,q8,q9}
  ...
  D4   = {q0,q13,q2,q3,q4,q5,q7,q8}   (accepting)

L(NFA) up to length 7 == L(DFA):  true   (31 strings)
  ...and == regex /(a|b)*abb/:  true

exponential blow-up:  NFA "the k-th symbol from the end is 'a'" (over {a,b})
    k  NFA states    DFA states    2^k
    1  2             2             2   (matches 2^k)
    2  3             4             4   (matches 2^k)
    3  4             8             8   (matches 2^k)
    4  5             16            16   (matches 2^k)
    5  6             32            32   (matches 2^k)

complete pipeline:  regex  ->  NFA (14)  ->  DFA (5)   -- and the DFA's language == the hand-written DFA from Module 10:  true
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out SubsetTest    # OK 88 passed
node js/subset.test.mjs         # OK 88 passed
```

---

## The algorithm

| Thing | Rule |
|---|---|
| DFA start state | `εClosure({ nfa.start })` |
| DFA `δ(S, a)` | `εClosure( ⋃ nfa.move(s, a) for s ∈ S )` |
| DFA state `S` accepting | `S ∩ nfa.accept ≠ ∅` |
| which states get built | only the reachable subsets (worklist) |

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `bad transition line: D0  D1` | a symbol in the emitted `.dfa` text was empty — the NFA's alphabet contained `""` | `rest()` must return `[]`, not `[""]`, for an empty `alphabet:` line (fixed in this module's copies) |
| DFA state count > `2^(NFA states)` | a subset was added twice under different keys | the set key must be order-independent — sort the states before joining |
| `L(NFA) != L(DFA)` | the `δ(S,a)` formula skipped an `εClosure` | it's `εClosure(move(...))`, closure on the *outside* |
| the k-th-from-end DFA has fewer than `2^k` states | you built a *different* NFA (e.g. missing the `s0 -a-> s0` self-loop) | see `kthFromEndNfa` — `s0` loops on both `a` and `b`, and *also* takes `a` to `s1` |
