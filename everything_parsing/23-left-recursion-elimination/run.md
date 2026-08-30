# Module 23 — Run It

From `23-left-recursion-elimination/`. Verified: OpenJDK 24, Node v22.14.0.

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
java -cp java/out LeftRecTest    # OK 11 passed
node js/leftrec.test.mjs          # OK 11 passed
```

---

## What the output shows

- **`expr.grammar`**: the natural `E -> E + T | T` is left-recursive; Paull's
  algorithm turns it into *exactly* the `E -> T E'` / `E' -> + T E' | epsilon`
  grammar Modules 19–22 have been using. That's where that grammar came from.
- **`indirect.grammar`** (Dragon Book 4.18): `A -> S d -> A c d` is left
  recursion with no single `A -> A...` rule. Paull substitutes `S` into `A`,
  then removes the now-direct recursion: `A -> b d A' | e A'`,
  `A' -> c A' | a d A' | epsilon`.
- **language check**: both grammars generate the identical set of strings up to
  length 6 — the transform preserves the language.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| still left-recursive after the transform | you eliminated direct recursion but not indirect | run the full Paull loop: substitute `Aj` (j<i) into `Ai` first |
| language changed | you dropped or duplicated a `beta` / `alpha` alternative | `A -> b A'` for every non-recursive `b`; `A' -> a A'` for every recursive `a`, plus `A' -> epsilon` |
| infinite loop in enumeration | grammar still has a cycle that adds no terminal | that's the bug the transform is supposed to fix — check `hasLeftRecursion` |
| new nonterminal name collides | `A'` already exists | append another `'` until the name is free |
