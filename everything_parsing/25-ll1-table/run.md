# Module 25 — Run It

From `25-ll1-table/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out LL1TableTest    # OK 19 passed
node js/ll1table.test.mjs          # OK 19 passed
```

---

## Reading the table

Rows are nonterminals, columns are terminals plus `$`. A cell holds the
**index** of the production to apply (see the numbered legend), `.` for a blank
(a syntax error when the parser lands there), `!` for a conflict.

- **`expr.grammar`**: every cell single-valued → LL(1). `M[Ep][)] = 2` and
  `M[Ep][$] = 2` come from `FOLLOW(Ep)`, not `FIRST`.
- **`danglingelse.grammar`**: `M[Sp][else]` is `!` — `Sp -> else S` and
  `Sp -> epsilon` both predict `else`.
- The trace at the end runs the `expr` table over `id + id * id` with an
  explicit stack — a preview of Module 26.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| a nullable production is missing from the table | you filled cells from FIRST only | for nullable `alpha`, also fill `M[A][t]` for `t in FOLLOW(A)` |
| `$` has no column | you built columns from terminals only | add `$` |
| conflict not detected | you overwrote the cell instead of appending | keep a list per cell; size > 1 = conflict |
| table has nonterminal columns | you iterated all symbols | columns are terminals (and `$`) only |
