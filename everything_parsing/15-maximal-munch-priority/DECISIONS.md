# Module 15 — Decisions

Choices in `Lexer.java` / `lexer.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The two rules

### Maximal munch — longest match wins — (b)
The universal lexer convention. Without it, `iffy` would tokenise as the keyword
`if` followed by an identifier `fy`, and `>=` would be `>` then `=`. Every real
language spec says "the next token is the longest sequence of characters that
forms a valid token." `longestAccept` implements it: keep scanning past the
first accepting state, remember the *last* one.

### Priority — earlier rule wins the tie — (b)
Also universal. `if` matches both the keyword pattern (length 2) and the
identifier pattern (length 2) — a tie. The keyword pattern is declared first, so
`if` is a keyword. This is *the* mechanism by which keywords are reserved. In
code it's one line: `nextToken` updates its best only on `len > bestLen`
(strictly greater), so the first rule to reach the winning length keeps it.

### The two rules interact in a fixed order — (a)
Length first, priority only to break a length tie. `iffy` doesn't become the
keyword `if` just because `KW_IF` has higher priority — priority never overrides
a longer match.

---

## The mechanism

### Each pattern gets its own minimal DFA — (c)
`regex → Thompson → subset → minimal` (Modules 12–14). `nextToken` runs all of
them from the current position and takes the max length. Module 16 combines them
into *one* DFA (tagged accepting states) so a single left-to-right pass handles
all patterns at once — the production design. Here, one DFA per rule keeps the
"two rules" visible and the code short.

### `longestAccept` returns the last-accept position, not the first — (a)
The whole point. It scans character by character; on every accepting state it
overwrites `lastAccept`; it only stops when the next character isn't in the
alphabet (or the input ends). Return value is the furthest accepting position,
or `-1`.

### A zero-length match is not a token — (c)
If `bestLen == 0` (every pattern matched only the empty prefix, or none matched),
`nextToken` returns `null` and `tokenize` raises a lexical error. A pattern like
`x*` matches `""` and would otherwise "consume" nothing forever. Module 16 adds
the explicit "a token pattern that matches ε is rejected at load time" check
(CSE 340's `epsilon IS NOOOOOT A TOKEN` error).

### A character outside every pattern's alphabet stops the scan and errors — (c)
`longestAccept` breaks out of its loop when `!alphabet.contains(sym)`; if that
leaves `bestLen == 0`, it's a lexical error at that position. A real lexer has a
whitespace rule and often a catch-all error rule; this demo has neither, on
purpose, so the error path is visible.

---

## The demo

### Alphabet `{i, f, x, y, =}`, four rules — (c)
Small enough that every DFA is tiny and traceable. `KW_IF`/`ID` demonstrate both
rules at once (`iffy` for munch, `if` for priority); `ASSIGN`/`EQ` show munch on
operators (`==` vs `=`), the single most common real-world case.

### `matchLengths` / the trace is for the tutorial only — (c)
It re-runs the scan collecting *every* accepting length, so the tutorial can
show `i(ACCEPT@1) if(ACCEPT@2) …`. Not used by the lexer itself.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Longest match wins | (b) | first match — `iffy` becomes `if` `fy` |
| 2 | Ties → first-declared rule | (b) | ties → last / most-specific — no spec does this |
| 3 | Length before priority | (a) | priority first — keywords would eat identifiers |
| 4 | One DFA per rule (here) | (c) | one combined tagged DFA — that's Module 16 |
| 5 | `longestAccept` tracks the LAST accept | (a) | stop at the first — breaks munch |
| 6 | Zero-length match ⇒ not a token ⇒ error | (c) | allow it — infinite loop on `x*` patterns |
| 7 | Out-of-alphabet char ⇒ stop + error | (c) | a catch-all rule — Module 16's job |

---

## What We Proved

1. **Longest match wins.** `iffy` tokenises as `ID "iffy"` even though the
   keyword pattern `if` matches a prefix. `==` is `ASSIGN`, not two `EQ`s.
   `===` is `ASSIGN` then `EQ`.

2. **Ties go to the earlier rule.** `if` matches both `KW_IF` and `ID` at
   length 2; it lexes as `KW_IF` because that rule is declared first. Swap the
   declaration order and the same input lexes as `ID` — verified.

3. **The mechanism is "DFA + last-accept mark."** Tracing `iffy`: the `KW_IF`
   DFA last-accepts at position 2, the `ID` DFA at position 4. The lexer takes
   the max, so the token is `ID "iffy"`, and scanning resumes at position 4.

4. **A full input tokenises deterministically.** `iffy==x=y` →
   `ID, ASSIGN, ID, EQ, ID`; `if==x` → `KW_IF, ASSIGN, ID`. A character no
   pattern accepts (a space) is a lexical error with an exact position.
