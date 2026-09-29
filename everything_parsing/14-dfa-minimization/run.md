# Module 14 — Run It

From `14-dfa-minimization/`. Verified: OpenJDK 24, Node v22.14.0. No fixture arg.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
subset-construction DFA for /(a|b)*abb/:  5 states

partition refinement -> 4 equivalence classes:
  { D0, D2 }
  { D1 }
  { D3 }
  { D4 }

table filling -> 4 classes;  same partition as refinement:  true

minimal DFA:  4 states
  state      a          b
  -> M0       M1         M0
     M1       M1         M2
     M2       M1         M3
    *M3       M1         M0

minimal DFA is isomorphic to the hand-written DFA:  true

--- regex equivalence (decided via minimal-DFA isomorphism) ---
  /(a|b)*/       == /(a*b*)*/      :  true   (both = Sigma*)
  /a**/          == /a*/           :  true
  /a(b|c)/       == /ab|ac/        :  true   (concatenation distributes over |)
  /ab|ba/        == /(a|b)(a|b)/   :  false  (RHS also matches aa, bb)
  /a*/           == /a+/           :  false  (a+ excludes the empty string)
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out MinimizeTest    # OK 31 passed
node js/minimize.test.mjs         # OK 31 passed
```

---

## The two algorithms

| Method | How |
|---|---|
| **partition refinement** (Moore) | start `{accepting}` \| `{non-accepting}`; each round, split a block whenever two members send some symbol to different blocks; stop when a round splits nothing |
| **table filling** | mark every pair `(p,q)` where exactly one is accepting; then repeatedly mark `(p,q)` if some symbol sends it to an already-marked pair; unmarked pairs are equivalent |

Both compute the identical partition. `minimal()` builds the quotient DFA and
names its states `M0…Mn` (start first).

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `bad transition line: q10   q11` | a regex string contained a **space** — Module 3's parser has no whitespace-skipping, so `' '` became a `Char` | remove spaces from regexes; `(ab)*ab | ()` → `(ab)*ab|()` |
| minimal DFA bigger than expected | unreachable states weren't trimmed first, or the initial partition didn't separate accepting from non-accepting | `trim()` runs first; the initial split is `{F} \| {Q\F}` |
| the two algorithms disagree | one didn't run to a fixed point | both loop until a full pass changes nothing |
| `equivalent(a, b)` wrong | the two minimal DFAs weren't compared for **isomorphism**, only state count | `isomorphic()` does a BFS pairing from the two start states |
