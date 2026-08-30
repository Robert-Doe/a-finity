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
--- input "x@@@#y" ---
  strategy PANIC_ONE (skip one bad char, ERROR per char):
    ID       "x" @0
    ERROR    "@" @1
    ERROR    "@" @2
    ERROR    "@" @3
    ERROR    "#" @4
    ID       "y" @5
    -> 4 lexical errors:  position 1 ('@'),  position 2 ('@'),  position 3 ('@'),  position 4 ('#')
  strategy PANIC_TO_SYNC (skip to a plausible token start):
    ID       "x" @0
    ERROR    "@@@#" @1
    ID       "y" @5
    -> 1 lexical error:  positions 1..4 ("@@@#")
  Module 16 (no recovery):  stops -- lexical error at position 1 (char '@')
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out RecoveryTest    # OK 18 passed
node js/recovery.test.mjs         # OK 18 passed
```

---

## The two strategies

| Strategy | On a lexical error | Result |
|---|---|---|
| `PANIC_ONE` | skip exactly one character, emit `ERROR` for it | one `ERROR` token per bad character |
| `PANIC_TO_SYNC` | skip characters until one that could start a token (in the alphabet **and** the DFA can move from its start on it) | one `ERROR` token spanning the whole garbage run |

Both do one scan and report **every** error; Module 16's `tokenize` still throws
at the first.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `PANIC_TO_SYNC` never syncs, one giant `ERROR` to end of input | no later character can start a token (all remaining chars are out-of-alphabet) | that's correct — the rest really is garbage |
| a valid character reported as an error | it's not in the spec's alphabet — the spec doesn't mention it at all | add a rule that covers it |
| recovery loops forever | `errEnd` didn't advance past `errStart` | `errEnd` starts at `pos + 1`, so at least one character is always consumed |
| `ERROR` tokens appear for a clean input | your "can start a token" check is wrong — it accepts positions the DFA is actually stuck at | `longestToken(lex, input, i)[0] > i` — a real match must start there |
