# Module 24 — Run It

From `24-left-factoring/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out LeftFactorTest    # OK 12 passed
node js/leftfactor.test.mjs          # OK 12 passed
```

---

## What the output shows

- **`danglingelse.grammar`**: `S -> if b then S | if b then S else S | other`
  factors to `S -> other | if b then S S'`, `S' -> epsilon | else S` — exactly
  the grammar Modules 20 and 22 analysed. (It still isn't LL(1) — the `else`
  conflict is real — but now the *rest* of the rule is factored.)
- **`nested.grammar`**: `a b c | a b d | a e` needs two passes — factor `a`,
  then factor `b` out of the stem.
- **`decl.grammar`**: three `id`-prefixed statements collapse to
  `stmt -> id stmt'`.
- **language check**: every grammar generates the identical string set before
  and after.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| grammar still `needsFactoring` after one call | you factored one prefix and stopped | loop until a full pass factors nothing |
| the empty suffix disappears | an alternative that *is* the prefix contributes `A' -> epsilon` | a zero-length suffix must become an explicit epsilon production |
| language changed | you moved an alternative into the wrong group | `withP` = every alt with the prefix; `rest` = the others; nothing dropped |
| infinite loop | you re-factor a length-0 "prefix" | only factor when the shared prefix has length ≥ 1 |
