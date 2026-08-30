# Module 26 — Run It

From `26-table-driven-parsing/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out TableParserTest    # OK 15 passed
node js/tableparser.test.mjs           # OK 15 passed
```

---

## What the output shows

Each `--- ... ---` block traces one parse:

- **`top` / `look`** — the stack top and the current lookahead at each step.
- **`expand ...`** — a table lookup fired; the RHS was pushed reversed.
- **`match`** — a terminal on the stack equalled the lookahead.
- **`ACCEPT`** — stack `$`, lookahead `$`.
- **`productions (a leftmost derivation)`** — the exact sequence of rules; the
  `replay` line applies them leftmost and confirms it reproduces the input.
- **`parse tree`** — built as a side effect, same shape as Module 19's.
- Rejected inputs print the trace up to the failing step and the position.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| parse loops forever | RHS pushed in forward order, or `A` not popped | pop `A`, push `Xn … X1` |
| tree missing the epsilon leaves | you skipped empty RHS | push nothing, but add an `epsilon` child node |
| `replay` disagrees with the input | you expanded a non-leftmost nonterminal | always expand the leftmost nonterminal in the form |
| trailing input accepted | no `$` sentinel | append `$` to the token stream and start the stack with `$` |
