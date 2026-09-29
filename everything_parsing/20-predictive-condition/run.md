# Module 20 — Run It

From `20-predictive-condition/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out PredictTest    # OK 22 passed
node js/predict.test.mjs          # OK 22 passed
```

---

## The four grammars in `fixtures/`

| File | FIRST/FOLLOW highlight | Verdict |
|---|---|---|
| `expr.grammar` | Module 19's expression grammar | **LL(1)** — every PREDICT set disjoint |
| `stmts.grammar` | nullable list `L -> S L \| epsilon`, `FOLLOW(L) = {$}` | **LL(1)** — `{id}` vs `{$}` don't meet |
| `prefix.grammar` | `FIRST(a b c) = FIRST(a b d) = {a}` | **not LL(1)** — left-factor (Module 24) |
| `danglingelse.grammar` | `FIRST(else S) = {else}`, `FOLLOW(X) ∋ else` | **not LL(1)** — nullable/FOLLOW clash on `else` |

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| FOLLOW set missing `$` | you didn't seed `FOLLOW(start) = {$}` | add it before iterating |
| FIRST never stabilises | you're re-adding on every pass without a change check | compare set size before/after; loop while any changed |
| a nullable rule reports no conflict when it should | you added `FIRST(a)` but not `FOLLOW(A)` for the nullable alternative | `PREDICT` must union `FOLLOW(A)` when `a` is nullable |
| left-recursive grammar shown as LL(1) | FIRST under-approximated and the overlap was hidden | left recursion is never LL(1); the check should still catch `A -> A x` vs `A -> y` |
