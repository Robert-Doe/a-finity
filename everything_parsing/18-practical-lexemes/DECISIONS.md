# Module 18 — Decisions

Choices in `PracticalLexer.java` / `practicallexer.mjs`, sorted into
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## Hand-coded, not generated

### This module drops the DFA generator — (c)
Modules 12–16 built a lexer from regexes. This one is a direct char-by-char
scanner over the cursor pattern. Why the switch: some of these lexemes
(**nested comments**) are *not regular*, so no regex/DFA can express them — you
need a hand-written exception. And a hand-written scanner is what real production
compilers (clang, rustc, go) actually ship, for speed and control over error
messages. Seeing both approaches is the point.

---

## Reserved words

### Scan an identifier, then look it up — (b), the standard trick
`identifierOrKeyword` scans `[A-Za-z_][A-Za-z0-9_]*`, then checks a `KEYWORDS`
set. `let` → `KW_LET`; `lettuce` → `ID`. Equivalent to Module 15's
priority-ordering, but with **one** identifier DFA instead of one DFA per
keyword — and adding a keyword is a one-word edit to a `Set`, not a rebuild.
Nearly every real lexer does it this way.

### Keyword kinds are `KW_` + the uppercased word — (c)
Deterministic, readable, and it keeps the keyword set and the token kinds in
sync automatically.

---

## Comments

### `//` and non-nested `/* */` are regular; nesting is not — (a)
`// [^\n]*` and `/\* (not */)* \*/` are regular expressions. **Nested** comments
are not: `/* (/* )ⁿ (*/ )ⁿ */` is the `aⁿbⁿ` pattern, provably outside regular
(Module 7). `skipBlockComment` handles nesting with `int depth` — one integer of
memory a finite automaton doesn't have. This is a deliberate, well-understood
step past regular, localised to one method.

### An unterminated comment / string reports the OPENING position — (c)
"unterminated block comment opened at position 0" — the position that matters
for the fix is where the construct *started*, not where the scanner gave up
(EOF). CSE-340-style precise error reporting.

### A line comment stops at `\n` or EOF; the `\n` is not consumed — (c)
So the main loop's whitespace handling skips the newline, and line/column
tracking (a later layer) stays correct.

---

## Strings

### Escapes are decoded into the token's value, not kept literal — (b)
`"a\nb"` yields a `STRING` token whose text is `a`, newline, `b` — three
characters. A parser/interpreter wants the *value*, not the source spelling.
Supported: `\n \t \" \\ \uXXXX`. An unknown escape (`\q`) is an error but the
scan continues.

### `\uXXXX` needs exactly four hex digits — (b)
Matching Java/JS/JSON. Fewer than four → error, and `'?'` is substituted so the
token is still produced.

### An unterminated string still yields a `STRING?` token — (c)
Kind `STRING?` (with the `?`) marks "the value we recovered, but it wasn't
closed." The token stream stays gap-free (Module 17's principle) and a
downstream tool can see both the partial value and the error.

### The string loop stops at `"`, `\n`, or EOF — (c)
A newline inside a `"..."` almost always means a missing closing quote, so
treating it as end-of-string (with an error) recovers better than consuming
across lines. Languages with multi-line strings use a different delimiter
(`"""`, backticks) and a different scanner path.

---

## Numbers

### One explicit state machine, with one retract — (a)
`INT` → optional `. digits` → `FLOAT` → optional `e [+-] digits` → `FLOAT`. The
retract: after consuming `e` (and an optional sign), if no digit follows, the
`e` wasn't an exponent — restore `pos` so `1e` lexes as `INT 1` then `ID e`.
This is Module 9's "read ahead, then back up" at the lexeme level.

### `0x` prefix branches early — (c)
`0` followed by `x`/`X` → hex mode, consume hex digits. `0xG` (no hex digits) is
an error but a `HEX` token is still emitted (empty-ish), keeping the stream
continuous.

### `12.` (trailing dot) is `INT 12`, the `.` is left for the next scan — (c)
`peek() == '.' && isDigit(peek(1))` — a `.` only joins the number if a digit
follows. `12.foo` → `INT 12`, then `.` (a lexical error here, no rule for it),
then `ID foo`. A real language with member access (`.`) makes `.` its own token.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Hand-coded scanner, no generator | (c) | reuse Module 16 — can't express nested comments |
| 2 | Reserved-word trick (scan + lookup) | (b) | one DFA per keyword — rebuild on every keyword |
| 3 | Nested comments via a depth counter | (a) | a regex — impossible, nesting isn't regular |
| 4 | Errors name the OPENING position | (c) | name the EOF position — useless for the fix |
| 5 | Decode string escapes into the value | (b) | keep them literal — the parser would have to re-decode |
| 6 | `STRING?` token for an unterminated string | (c) | drop it — parser loses coverage and the partial value |
| 7 | Number state machine with `e`-retract | (a) | greedy `e` — `1e` swallows a following identifier |
| 8 | `12.` → `INT 12`, `.` left behind | (c) | `12.` → `FLOAT` — ambiguous with member access |

---

## What We Proved

1. **The reserved-word trick works.** `let x in y` → `KW_LET ID KW_IN ID`;
   `lettuce` and `if1` are plain `ID`s. One identifier scanner plus a
   seven-word table.

2. **Nested comments are not regular, and a counter fixes it.**
   `a /* outer /* nested */ still comment */ b` → just `a` and `b`. A flat regex
   `/\*.*\*/` would close at the first `*/` and leave `still comment */ b`
   dangling. `/* /* only one close */` → "unterminated block comment opened at
   position 0."

3. **Strings decode their escapes.** `"hi\nthere\t!"` → a `STRING` token
   containing a real newline and tab. `AB` → `AB`. An unterminated
   string → an error at the opening position plus a `STRING?` token.

4. **Numbers are a small state machine.** `12` → `INT`, `3.14` / `1e10` /
   `1.5e-3` → `FLOAT`, `0xFF` → `HEX`. `1e` retracts to `INT 1` + `ID e`.
