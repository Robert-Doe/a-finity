# Module 01 — Run It

All commands run from this directory (`01-syntax-vs-semantics/`).
PowerShell and bash are identical here.

Toolchain verified: **OpenJDK 24**, **Node v22.14.0**. Minimums: JDK 17, Node 20.

---

## 1. Java — build, then run the demo

```
javac -d java/out java/*.java
java -cp java/out Main fixtures/inputs.txt
```

Expected output (byte-for-byte — this is `expected/main.out`):

```
=== Module 01 - Syntax vs. Semantics - RPN checker ===

[1] "3 4 +"
    syntax   : PASS
    semantics: PASS  value = 7

[2] "3 4 + +"
    syntax   : FAIL  col 7: operator '+' needs 2 operands, stack has 1
    semantics: (skipped: syntax failed)

[3] "3 4"
    syntax   : FAIL  2 values left on stack, expected 1
    semantics: (skipped: syntax failed)

[4] "12 0 /"
    syntax   : PASS
    semantics: FAIL  col 6: division by zero

[5] "10 2 - 3 *"
    syntax   : PASS
    semantics: PASS  value = 24

[6] "3 x +"
    syntax   : FAIL  col 3: unknown token 'x'
    semantics: (skipped: syntax failed)

[7] ""
    syntax   : FAIL  empty expression
    semantics: (skipped: syntax failed)

summary: inputs=7  syntaxPass=3  semanticsChecked=3  semanticsPass=2  syntaxOkButSemanticFail=1
```

The line that matters: **`[4] "12 0 /"` — `syntax : PASS`, `semantics : FAIL`.**
One string, two verdicts.

---

## 2. JavaScript — run the demo

```
node js/main.mjs fixtures/inputs.txt
```

Produces exactly the same bytes as the Java build.

---

## 3. Tests

```
java -cp java/out RpnTest
node js/rpn.test.mjs
```

Each ends with:

```
OK   28 passed
```

Exit code is 0 on pass, 1 on any failure.

---

## 4. Compare the two builds yourself

```
java -cp java/out Main fixtures/inputs.txt > /tmp/java.out
node js/main.mjs fixtures/inputs.txt > /tmp/js.out
diff /tmp/java.out /tmp/js.out    # no output = identical
```

(PowerShell: `Compare-Object (Get-Content java.out) (Get-Content js.out)`.)

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `cannot find symbol: Assert` | compiled one file, not the folder | `javac -d java/out java/*.java` — keep the glob |
| `NoSuchFileException: fixtures/inputs.txt` | ran from the wrong directory | `cd` into `01-syntax-vs-semantics/` first |
| Java output has `^M` / blank lines double-spaced in a diff | your editor rewrote `expected/main.out` with CRLF | re-generate: `java -cp java/out Main fixtures/inputs.txt > expected/main.out` |
| `SyntaxError: Unexpected token` from Node | Node < 14, or file saved as `.js` not `.mjs` | use Node 20+, keep the `.mjs` extension |
| `summary: inputs=0` | the quotes were stripped from `fixtures/inputs.txt` | every input line must stay wrapped in `"` — unquoted lines are skipped as non-input |
