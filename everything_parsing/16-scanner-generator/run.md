# Module 16 — Run It

From `16-scanner-generator/`. Verified: OpenJDK 24, Node v22.14.0.
Capstone A: generate Ajoda's lexer from a token spec.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
load checks: 28 patterns parsed, none accepts the empty string
combined DFA: 287 states over 97 input symbols

--- fixtures/factorial.ajoda ---
  2:1    FN        fn
  2:4    IDENT     fact
  2:8    LPAREN    (
  ...
  9:1    RBRACE    }
  10:1   EOF

> iffy while1 truest _if
  IDENT(iffy) IDENT(while1) IDENT(truest) IDENT(_if) EOF

> 3.14 42 7.
  FLOAT_LIT(3.14) INT_LIT(42) INT_LIT(7)
  error 1:10: no token can start with '.'

--- fixtures/broken.tokens ---
  refused: spec line 3: SIGN accepts the empty string; a token must consume at least one character
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out ScannerTest      # OK 18 passed
node js/scanner.test.mjs           # OK 18 passed
```

---

## Spec file format

```
# a whole-line comment
token    NAME  pattern      # emit a NAME token
discard  NAME  pattern      # match, then drop (whitespace, comments)
keywords FROM  w1 w2 ...    # a FROM lexeme equal to a word becomes that keyword
```

Pattern syntax:

| Write | Means |
|---|---|
| `abc` | the characters a, b, c in order |
| `a\|b`, `a*`, `a+`, `a?`, `( )` | union, repetition, grouping |
| `[a-z_]`, `[^\n]` | character class, negated class |
| `"->"` | a quoted literal; no escaping needed inside |
| `\(` `\*` `\s` `\t` `\n` | escaped metacharacter, space, tab, newline |

Ties between equally long matches go to the rule declared first.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `X accepts the empty string` | the whole pattern is optional: `a?`, `[0-9]*`, `()` | make at least one piece required |
| `bare whitespace at column N` | a literal space inside a pattern | use `\s`, `\t` or `" "` |
| `'(' at column N has nothing to apply to` | a metacharacter used as a literal | escape it (`\(`) or quote it (`"("`) |
| a keyword comes out as `IDENT` | missing from the `keywords` line | add it, and check the line names `IDENT` |
| `<=` scans as `<` then `=` | the scan stops at the first accept | keep stepping; back up only to the *last* accept |
| columns drift after a comment | discarded lexemes don't advance `col` | walk every lexeme, discarded or not |
