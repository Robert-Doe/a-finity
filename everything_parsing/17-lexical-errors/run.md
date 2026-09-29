# Module 17 — Run It

From `17-lexical-errors/`. Verified: OpenJDK 24, Node v22.14.0.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
--- probe: total = a @@@# b; ---
  SKIP_ONE (one ERROR per bad character):
    IDENT(total) ASSIGN(=) IDENT(a) ERROR(@) ERROR(@) ERROR(@) ERROR(#) IDENT(b) SEMI(;) EOF
    4 errors: 1:11 "@", 1:12 "@", 1:13 "@", 1:14 "#"
  SKIP_TO_RESTART (one ERROR per bad run):
    IDENT(total) ASSIGN(=) IDENT(a) ERROR(@@@#) IDENT(b) SEMI(;) EOF
    1 error: 1:11 "@@@#"
  Module 16 alone: stops at 1:11: no token can start with '@'

--- fixtures/typos.ajoda, SKIP_TO_RESTART ---
  32 tokens, 3 errors:
    2:20 "$"
    3:20 "??"
    5:14 "@"
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out RecoveryTest    # OK 15 passed
node js/recovery.test.mjs         # OK 15 passed
```

---

## The two strategies

| Strategy | On a lexical error | Result |
|---|---|---|
| `SKIP_ONE` | skip exactly one character, emit `ERROR` for it | one `ERROR` token per bad character |
| `SKIP_TO_RESTART` | skip characters until some token can start (`longestMatch` is not null) | one `ERROR` token spanning the whole bad run |

Both do one scan and report **every** error with `line:col`; Module 16's
`scan` still stops at the first.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `SKIP_TO_RESTART` never restarts, one giant `ERROR` to end of input | no later position can start a token | that's correct; the rest really is garbage |
| characters inside a `//` comment are reported | the comment rule isn't matching | check the `discard COMMENT` pattern in `ajoda.tokens` |
| recovery loops forever | `end` didn't advance past `pos` | `end` starts at `pos + 1`, so at least one character is always consumed |
| error columns drift after the first error | the skipped text didn't advance `line:col` | advance over `ERROR` text like any other lexeme |
