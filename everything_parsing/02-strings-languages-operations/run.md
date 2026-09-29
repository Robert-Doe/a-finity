# Module 02 — Run It

From `02-strings-languages-operations/`. Verified: OpenJDK 24, Node v22.14.0.
This module has **no fixture file** — the "input" is the algebra itself.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`):

```
=== Module 02 - Alphabets, Strings, Languages, Operations ===

alphabet Sigma = { a, b }
Sigma* up to length 3  (bounded Kleene star of the alphabet):
  { epsilon, a, b, aa, ab, ba, bb, aaa, aab, aba, abb, baa, bab, bba, bbb }   [15 strings]

L1 = { a, b }
L2 = { c }
L1 union L2  = { a, b, c }
L1 concat L2 = { ac, bc }
L2 concat L1 = { ca, cb }

closure of finite languages:
  L1 union L2 : 3 strings
  L1 concat L2: 2 strings
  L1 power 3  : { aaa, aab, aba, abb, baa, bab, bba, bbb }  (8)

the empty string is not the empty language:
  {}          size 0
  { epsilon }  size 1
  {} concat L1        = { }   (annihilator)
  { epsilon } concat L1 = { a, b }   (identity)

Kleene star of { a } up to length 4:  { epsilon, a, aa, aaa, aaaa }   [5 strings]

regex as operations:  (a|b) a*   up to length 4
  built as: ( {a} union {b} ) concat ( {a}.star(3) )
  = { a, b, aa, ba, aaa, baa, aaaa, baaa }   [8 strings]
  membership:  "b" in L   "ba" in L   "ab" not in L   "" not in L

summary: operations=union,concat,power,star  epsilonLang!=emptyLang=true  regexReproduced=true
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out LanguageTest
node js/language.test.mjs
```

Both end `OK   23 passed`.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `star` never returns | you passed a huge `maxLen` and a language with many strings — `\|Sigma*\|` grows as `\|Sigma\|^len` | keep bounds small; length 6 over a 4-symbol alphabet is already 5461 strings |
| `power exponent must be >= 0` | called `power(-1)` | powers are non-negative; `power(0)` is `{ epsilon }` |
| render order differs from the doc | a hand-rolled comparator sorting purely lexicographically | strings sort by **length first**, then lexicographically — `"aa"` before `"ab"`, both before `"aaa"`? no: `"aaa"` (len 3) comes after both len-2 strings |
| `{ epsilon }` and `{}` look the same in your code | you stored the empty string as `null` or skipped it | the empty string `""` is a value; `{}` is a set with zero elements, `{ "" }` has one |
