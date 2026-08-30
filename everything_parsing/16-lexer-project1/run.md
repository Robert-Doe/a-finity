# Module 16 — Run It

From `16-lexer-project1/`. Verified: OpenJDK 24, Node v22.14.0.
This is the equivalent of ASU CSE 340's **Project 1**.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
spec: fixtures/mini.spec
  (0) KW_LET   = let
  (1) KW_IN    = in
  (2) ID       = (e|i|l|n|t|x|y)(e|i|l|n|t|x|y)*
  (3) NUM      = (0..9)(0..9)*
  ...
  (7) WS       = .   [skipped]

epsilon check:  no token can match the empty string  ->  OK
combined DFA:  43 states  (union of 8 pattern NFAs, tagged, determinized)

"let.x=12.in.x"
  KW_LET   "let" @0
  ID       "x" @4
  ASSIGN   "=" @5
  NUM      "12" @6
  KW_IN    "in" @9
  ID       "x" @12
  (3 WS tokens skipped)

"let.let.in"
  KW_LET   "let" @0
  KW_LET   "let" @4
  KW_IN    "in" @8

--- bad spec: a token that matches epsilon ---
  fixtures/bad.spec  ->  rejected:  token 'MAYBE' can match the empty string  (epsilon IS NOOOOOT A TOKEN)
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out SpecLexerTest    # OK 15 passed
node js/speclexer.test.mjs         # OK 15 passed
```

---

## Spec file format

```
# comments to end of line
NAME    regex          # one per line, in PRIORITY order (keywords before ID)
%skip   NAME           # tokens of this kind are consumed but not emitted
```

The regex uses Module 3's syntax (`( ) | * + ?`, no escaping, no character
classes). Because there's no escaping, a token whose literal is a regex
metacharacter can't be spelled directly — the demo language avoids
`( ) | * + ?` as single-character tokens and uses `.` for whitespace.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `token 'X' can match the empty string` | a pattern is nullable — `a?`, `(a\|b)*`, `()` as a whole token | a token must consume ≥ 1 character; wrap the nullable part so the whole pattern isn't |
| `'(' has nothing to apply to` while loading | a token literal is a regex metacharacter | those can't be spelled without escaping (Module 3's limit) |
| a keyword lexes as `ID` | `ID` is declared before the keyword | keywords must be earlier in the file (priority) |
| `<=` lexes as `<` then `=` | the combined DFA isn't tracking the longest accept | `tokenize` records `(lastPos, lastRule)` and only stops when the DFA can't advance |
| token positions are "wrong" after whitespace | you're counting emitted tokens, not source characters | `pos` advances by the lexeme length including skipped tokens |
