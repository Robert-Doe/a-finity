# Module 27 — Run It

From `27-ll1-error-recovery/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out RecoveringParserTest    # OK 17 passed
node js/recoveringparser.test.mjs           # OK 17 passed
```

---

## What the output shows

Each input is parsed with recovery; the trace shows every recovery action:

| Action in the trace | What happened |
|---|---|
| `discard 'X'` | panic mode: `X` isn't in the sync set, drop it and retry |
| `pop A (sync on FOLLOW)` | panic mode: lookahead is in `FOLLOW(A)`, skip the whole nonterminal |
| `insert 'X' (phrase-level)` | a terminal `X` was expected but missing; assume it and continue |
| `discard (extra input)` | tokens after a complete parse |

Every listed input **recovers to end of input** and reports its errors with
positions. `id + id * id` is clean (0 errors).

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| recovery loops forever | a branch neither pops the stack nor advances the input | panic-on-blank must do exactly one of: pop `A`, or advance past the token |
| every token after an error is flagged | you don't re-sync, you just skip one and stop | keep parsing; the guard is "reached end of input" |
| missing-token errors never fire | terminal mismatch falls through to panic mode | handle "terminal on top ≠ lookahead" as phrase-level insertion first |
| sync set too aggressive | you used FIRST∪FOLLOW everywhere | FOLLOW(A) is the standard synchronizing set for skipping `A` |
