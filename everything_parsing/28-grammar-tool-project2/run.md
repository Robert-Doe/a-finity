# Module 28 — Run It  (CSE 340 Project 2)

From `28-grammar-tool-project2/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out GrammarToolTest    # OK 18 passed
node js/grammartool.test.mjs           # OK 18 passed
```

---

## The seven sections

For each grammar in `fixtures/`, the tool prints:

| # | Section | Component | From |
|---|---------|-----------|------|
| 1 | terminals / nonterminals in appearance order, start | `Grammar` | Module 4 |
| 2 | nullable nonterminals | `FirstSets` | Module 21 |
| 3 | FIRST sets | `FirstSets` | Module 21 |
| 4 | FOLLOW sets | `FollowSets` | Module 22 |
| 5 | left-recursion-free grammar (Paull) | `LeftRec` | Module 23 |
| 6 | left-factored grammar | `LeftFactor` | Module 24 |
| 7 | LL(1) verdict + parsing table (or conflicts + "does the fix work?") | `Predict`, `LL1Table` | Modules 20, 25 |

- **`stmts.grammar`** — already LL(1); section 7 prints the table.
- **`expr.grammar`** — left-recursive, 6 conflicts; section 5 produces the
  Module 19 grammar and section 7 confirms the transformed grammar is LL(1).
- **`json.grammar`** — no left recursion but shared prefixes; section 6
  left-factors it and section 7 confirms LL(1) after the fix.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| section 5 bloats a non-left-recursive rule | Paull substituting when it shouldn't | only substitute `Aj` into `Ai` when `Aj` can leftmost-reach `Ai` |
| "after ... LL(1) = NO" for a fixable grammar | you factored *or* removed recursion, not both, or in the wrong order | remove left recursion first, then left-factor |
| FOLLOW section wrong | FIRST wrong (section 3) | FOLLOW is built on FIRST — fix section 3 first |
| the transformed grammar can't be re-analysed | `A'` names not accepted by the loader | the loader must accept apostrophes in nonterminal names |
