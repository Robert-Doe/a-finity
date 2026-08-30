# Module 30 — Run It

From `30-shift-reduce-handles/`. Verified: OpenJDK 24, Node v22.14.0.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

## JavaScript — identical bytes

```
node js/main.mjs
```

## Tests

```
java -cp java/out ShiftReduceTest    # OK 14 passed
node js/shiftreduce.test.mjs           # OK 14 passed
```

---

## Reading the trace

| Column | Meaning |
|---|---|
| STACK | grammar symbols on the stack, bottom → top (a **viable prefix**) |
| ACTION | `shift X`, `reduce A -> b (handle: b)`, or `ACCEPT` |
| INPUT | tokens not yet read, then `$` |

- **`expr`** parses the *left-recursive* grammar directly — no transform.
  `T -> T * F` is reduced before `E -> E + T`, so `*` binds tighter.
- The **reductions reversed** are printed as a rightmost derivation; the last
  line confirms it ends at the input.
- **`ambiguous`** (`E -> E + E`) reaches stack `[E + E]` with `+` ahead where
  *both* reducing and shifting reach ACCEPT — a **shift-reduce conflict**, and
  two handles.

This module finds handles by bounded DFS (no table). Module 31 builds the DFA.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| "no parse found" for valid input | DFS budget too small, or a reduce/shift branch missing | raise the cap; try every reduction whose RHS is a stack suffix, then shift |
| reversed reductions aren't a rightmost derivation | you expanded the leftmost `A`, not the rightmost | replace the **rightmost** occurrence of the LHS |
| over-shifts and never reduces | you only ever shift | try reductions first, backtrack from dead ends |
| conflict not detected on an ambiguous grammar | the probe only checks one move | at each reachable state, count how many distinct moves reach ACCEPT |
