# Module 15 — Run It

From `15-maximal-munch-priority/`. Verified: OpenJDK 24, Node v22.14.0. No fixture arg.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
token rules  (priority = declaration order):
  (0) KW_IF    = if
  (1) ID       = (i|f|x|y)(i|f|x|y)*
  (2) ASSIGN   = ==
  (3) EQ       = =

RULE 1 -- longest match wins:
  "iffy"   -> ID "iffy" @0   (KW_IF/EQ would have matched a shorter prefix)

RULE 2 -- on a length tie, the EARLIER rule wins:
  "if"   -> KW_IF matches len 2,  ID matches len 2  -- tie  ->  KW_IF "if" @0   (rule 0 < rule 1)

the mechanism -- run the DFA, keep the LAST-ACCEPT position, back up to it:
  KW_IF    on "iffy":  i(-) if(ACCEPT@2) iff(-) iffy(-)   -> last accept = 2
  ID       on "iffy":  i(ACCEPT@1) if(ACCEPT@2) iff(ACCEPT@3) iffy(ACCEPT@4)   -> last accept = 4

full tokenization:
  input "iffy==x=y":
    ID "iffy" @0
    ASSIGN "==" @4
    ID "x" @6
    EQ "=" @7
    ID "y" @8

lexical error demo:  tokenize("if z")  ->  lexical error at position 2 (char ' ')
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out LexerTest    # OK 17 passed
node js/lexer.test.mjs         # OK 17 passed
```

---

## The two rules

| Rule | Statement | Example |
|---|---|---|
| **1 — maximal munch** | the token is the longest prefix any pattern matches | `iffy` → `ID`, not `KW_IF` + `fy` |
| **2 — priority** | on a length tie, the pattern **declared first** wins | `if` → `KW_IF` (declared before `ID`), not `ID` |

Mechanism (`longestAccept`): run the pattern's DFA from the current position;
every time it enters an accepting state, record the position; when it can't
advance, the last recorded position is the match length.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `lexical error at position N` | the character at `N` is in no pattern's alphabet | add a rule for it (whitespace, punctuation, …) — Module 16 does the full spec |
| a keyword lexes as an identifier | the identifier rule is declared before the keyword rule | keywords must come first (RULE 2) |
| `iffy` lexes as `if` + `fy` | you compared *first* match instead of *longest* | track the last-accept position, don't stop at the first accept |
| `==` lexes as `=` + `=` | same — you stopped at the first accepting state | keep scanning; `==` accepts at length 2 |
| a rule whose regex matches the empty string swallows nothing forever | `nextToken` correctly rejects a zero-length match (`bestLen == 0` → null) | don't write `x*` as a whole token pattern; Module 16's "epsilon is not a token" check |
