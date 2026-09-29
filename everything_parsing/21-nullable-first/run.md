# Module 21 — Run It

From `21-nullable-first/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out FirstSetsTest    # OK 23 passed
node js/firstsets.test.mjs          # OK 23 passed
```

---

## What the output shows

- **NULLABLE iteration** — round 0 is `{ }`; each round adds nonterminals whose
  every production either is `epsilon` or has an all-nullable right-hand side.
  `indirect.grammar` needs three productive rounds (`Z`, then `Y`, then `X`).
- **FIRST iteration** — round 0 all empty; `expr.grammar` shows `FIRST(E)` stay
  empty for two rounds because it has to wait for `T`, which waits for `F`.
- **`one more pass changes nothing? true`** — the result is a fixed point.
- **cross-check vs Module 20** — this module's FIRST and Module 20's independent
  `Predict.firstOf` agree on every nonterminal.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `A -> epsilon` doesn't make `A` nullable | empty rhs not handled | an empty rhs means the "all symbols nullable" check is vacuously true |
| nullable misses an indirect case | you ran one pass, not to convergence | loop while any nonterminal was added |
| `FIRST` missing tokens after a nullable symbol | you stopped at the first nonterminal | keep going while the current symbol is nullable |
| `epsilon` leaks into `FIRST(S)` when `S` isn't nullable | you added `epsilon` without checking the whole rhs | only add `epsilon` if every symbol in the rhs is nullable |
