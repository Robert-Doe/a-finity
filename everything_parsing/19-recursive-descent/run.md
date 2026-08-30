# Module 19 — Run It

From `19-recursive-descent/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out RecursiveDescentTest    # OK 21 passed
node js/recursivedescent.test.mjs          # OK 21 passed
```

---

## What to look for in the output

| Section | The point |
|---|---|
| `productions fired, in order` | one entry per nonterminal method call, in call order |
| `replayed as a leftmost derivation` | the *same list* drives `E => … => sentence`; it only ever expands the leftmost nonterminal |
| `match? true` | the derived sentence equals the input's token kinds — the parse is correct |
| `concrete syntax tree` | `*` sits under a `T'` (tighter than `+`); `a + b + c` leans right |
| `SYNTAX ERROR at position N` | `expect()` failed; the position is the offending token's start |

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `StackOverflowError` / `RangeError` | you tried a left-recursive rule (`E -> E + T`) | recursive descent needs the non-left-recursive form — Module 23 |
| `replay` throws "not leftmost" | the production list isn't a real call order | each method must `rules.add(...)` *before* recursing |
| parse succeeds on `1 2` | you forgot `expect("EOF")` after `parseE()` | a complete parse must consume every token |
| error position is always 0 | you're reading `pos` off the wrong token | report `peek().pos` at the moment `expect` fails |
