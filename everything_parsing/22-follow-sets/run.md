# Module 22 — Run It

From `22-follow-sets/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out FollowSetsTest    # OK 18 passed
node js/followsets.test.mjs          # OK 18 passed
```

---

## What the output shows

- **round 0** seeds `$` into `FOLLOW(start)` — nothing else.
- **`abc.grammar`**: `FOLLOW(A) = {b, c}` — `A` is followed by `B` (contributing
  `b`), and `B` is nullable so `c` flows through too.
- **`danglingelse.grammar`**: `FOLLOW(S)` and `FOLLOW(X)` are mutually
  dependent; the iteration resolves the cycle to `{else, $}` for both.
- **`expr.grammar`**: `FOLLOW(F)` collects `)` (from `F -> ( E )`), `*` (from
  `Tp -> * F Tp`), `+` (through `Tp` being nullable), and `$`.
- **cross-check** — agrees with Module 20's independent `Predict.followOf`.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `FOLLOW(start)` missing `$` | not seeded | add `$` before the first pass |
| operator token missing from a `FOLLOW` set | you didn't apply rule 3 for a nullable tail | when `β` is nullable, add `FOLLOW(lhs)` to `FOLLOW(A)` |
| `epsilon` shows up in a `FOLLOW` set | you added raw `FIRST(β)` | strip `epsilon`; it's `FIRST(β) \ {epsilon}` |
| dangling-else `FOLLOW` sets disagree | you stopped after one pass | iterate until a pass changes nothing |
| results differ from Module 20 | FIRST input is wrong | FOLLOW is only as right as the FIRST it's built on (Module 21) |
