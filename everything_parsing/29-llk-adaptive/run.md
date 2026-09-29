# Module 29 — Run It

From `29-llk-adaptive/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out LlkTest    # OK 14 passed
node js/llk.test.mjs           # OK 14 passed
```

---

## What the output shows

**Part 1 — FIRST_k and the minimal k**

| Grammar | FIRST_1 | FIRST_2 | minimal k |
|---|---|---|---|
| `expr` | disjoint | — | **1** |
| `label` | `stmt` alts both `{id}` — collision | `{id colon}` vs `{id lparen, id semi}` — disjoint | **2** |
| `equiv` | `A -> x B` and `A -> x C` share `{x}` | still identical (B and C both derive A) | **none** — ambiguous |

**Part 2 — backtracking cost**

- `equiv` on `x^n y`: parse trees = 2ⁿ, production tries ≈ 2ⁿ⁺¹. No lookahead
  can prune because the two alternatives are genuinely equivalent.
- `expr` on `id (+ id)ⁿ`: exactly 1 parse tree, tries grow **linearly** — the
  wrong alternative of each nonterminal fails on the first token.

**LL(\*)** — ANTLR's adaptive prediction: a DFA over the lookahead reads as many
tokens as needed (regular, not bounded), backtracking only if the DFA can't
decide.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| FIRST_1 has spurious extra tokens / `()` | an empty (not-yet-computed) nonterminal set treated as nullable | an empty symSet contributes nothing; only a real `epsilon` production puts `""` in the set |
| `minLL` says 1 for a non-LL(1) grammar | you used the FIRST_k test for k=1 instead of the exact PREDICT check | delegate k=1 to `Predict.isLL1()` |
| backtracking never terminates | the grammar has left recursion | remove it first (Module 23), or the recognizer recurses forever |
| parse counts overflow | very long input on an exponential grammar | that's the point — cap `entries` and report "capped" |
