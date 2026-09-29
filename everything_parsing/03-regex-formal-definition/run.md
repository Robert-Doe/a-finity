# Module 03 — Run It

From `03-regex-formal-definition/`. Verified: OpenJDK 24, Node v22.14.0.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main fixtures/regexes.txt
```

Expected (excerpt — full text in `expected/main.out`):

```
=== Module 03 - Regular Expressions: Formal Definition ===

[1] /(a|b)*abb/
    tree : (cat (cat (cat (star (alt a b)) a) b) b)
    L (up to length 6): { abb, aabb, babb, aaabb, ababb, baabb, bbabb, aaaabb, aababb, abaabb, abbabb, baaabb, ... (15 total) }
    "abb"     match
    "aabb"    match
    "babb"    match
    "abababb" match
    "ab"      no
    "abba"    no
    ""        no

...

precedence check (tightest: * , then concatenation, then |):
    /a|bc*     ->  (alt a (cat b (star c)))
    /ab*       ->  (cat a (star b))
    /(ab)*     ->  (star (cat a b))
    /a|b|c     ->  (alt (alt a b) c)
    /ab|cd     ->  (alt (cat a b) (cat c d))

summary: regexes=5  nodeKinds=6 (Empty, Epsilon, Char, Union, Concat, Star)  sugar: + ? desugared at parse time
```

---

## JavaScript — identical bytes

```
node js/main.mjs fixtures/regexes.txt
```

---

## Tests

```
java -cp java/out RegexTest
node js/regex.test.mjs
```

Both end `OK   34 passed`.

---

## Fixture format

`fixtures/regexes.txt` — one line per regex:

```
REGEX | test1 test2 test3
```

The separator is a **space-pipe-space** (` | `). Regex alternation (`a|b`) has
no spaces, so it never collides. Use `~` for the empty test string.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `unexpected '\|' at position 0` | the whole line was treated as a regex — no ` \| ` separator found | put a space on each side of the separating pipe |
| `'*' has nothing to apply to` | regex starts with a quantifier, e.g. `*abc` | quantifiers are postfix; they need an atom to their left |
| `missing ')'` | unbalanced parentheses | every `(` needs a `)` |
| a match you expect fails | precedence — `ab*` is `a(b*)`, not `(ab)*`; `a|bc` is `a\|(bc)`, not `(a\|b)c` | add parentheses; check the `tree:` line in the output |
| `matches` hangs on `(a*)*`-style input | you dropped the "a repetition must consume ≥ 1 char" guard in the Star case | keep the `rest != w` check in `leftovers` |
