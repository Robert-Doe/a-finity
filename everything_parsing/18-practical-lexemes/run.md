# Module 18 — Run It

From `18-practical-lexemes/`. Verified: OpenJDK 24, Node v22.14.0.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
keyword table: else false if in let then true

--- msg = "hi\nthere\t!" ---
  tokens:  ID "msg"  OP "="  STRING "hi\nthere\t!"

--- a /* outer /* nested */ still comment */ b ---
  tokens:  ID "a"  ID "b"

--- c // rest of line ignored~d = 4 ---
  tokens:  ID "c"  ID "d"  OP "="  INT "4"

--- oops = "no closing quote ---
  tokens:  ID "oops"  OP "="  STRING? "no closing quote"
  ERROR:   unterminated string literal opened at position 7

nesting is beyond regular:
  "/* a /* b */ c */ x"  ->  ID "x"
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out PracticalLexerTest    # OK 30 passed
node js/practicallexer.test.mjs         # OK 30 passed
```

---

## What's hand-coded here (and why)

| Lexeme | Approach | Regular? |
|---|---|---|
| keywords | scan `[A-Za-z_][A-Za-z0-9_]*`, then look up in `KEYWORDS` | yes — one identifier DFA + a table |
| `//` line comment | scan to `\n` | yes |
| `/* */` block comment | scan to `*/` | yes |
| **nested** `/* /* */ */` | scan with a **depth counter** | **no** — counting is beyond finite state |
| string `"..."` | scan char-by-char, interpret `\n \t \" \\ \uXXXX` | yes (the escape set is finite) |
| numbers `1e10`, `0xFF` | a small explicit state machine with one retract | yes |

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| a keyword lexes as `ID` | it's not in the `KEYWORDS` set | add it — that's the whole point of the trick |
| `1e` becomes one token | you didn't retract when `e` isn't followed by a digit | save `pos` before consuming `e`; restore if no exponent digits follow |
| nested `/* */` leaves trailing garbage | you're matching to the first `*/`, not counting | use `depth++` on `/*`, `depth--` on `*/`, stop at `depth == 0` |
| `"a\nb"` has a literal backslash-n in the value | you appended the escape characters instead of interpreting them | on `\`, read the next char and append the *decoded* character |
| unterminated string hangs or eats the rest of the file | no `\n` / EOF guard in the string loop | stop at `"`, `\n`, or EOF; report the opening position |
