# Module 16 — Decisions

Choices in `SpecLexer.java` / `speclexer.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The spec file

### One `NAME  regex` per line, priority = file order — (b), CSE 340 contract
The whole point of Project 1: the lexer is *data-driven*. Change the spec, get a
different lexer, no code change. Order matters — earlier lines win length ties
(Module 15's Rule 2), which is how keywords are reserved.

### `%skip NAME` for tokens that are consumed but not emitted — (c)
Whitespace and comments are real tokens (they match, they advance the cursor,
they count toward positions) but the parser shouldn't see them. A `%skip` line
marks a token kind for discard after matching. This keeps the scanner uniform —
everything is a token — while giving the parser a clean stream.

### `.` is the demo's whitespace character — (c), forced by Module 3's regex
Module 3's regex has **no escaping**. A whitespace pattern like `( |\t)+` can't
be written (`\t` isn't a thing, and a literal space would be trimmed by the
spec parser). So the toy language uses `.` where a real one uses space, and
`WS = .` with `%skip WS`. A production lexer's regex layer has escapes and
character classes; ours doesn't, and the demo works around it rather than
complicate Module 3.

---

## Building the combined automaton

### One NFA per pattern (Thompson), UNIONed under a fresh start, with tags — (a)
`START -ε-> r0_start`, `START -ε-> r1_start`, … Each pattern's single Thompson
accept state is tagged with its rule index in `acceptRule`. This is the standard
"combine all token DFAs into one" construction.

### Determinize the union; each DFA state's token = min tag in its subset — (a)
`Subset.determinize` gives the DFA; `Subset.legend` gives each DFA state's
subset of NFA states. A DFA state is accepting for token `k` where `k` is the
*smallest* rule index among the tagged NFA states in its subset — smallest index
= highest priority = Rule 2, baked into the DFA. One left-to-right pass then
handles maximal munch for *all* patterns at once: O(input), independent of the
number of rules.

### The combined DFA is **not** minimized — (c)
Minimizing it (Module 14) would shrink 43 states, but naive minimization merges
accepting states with *different token tags* — collapsing "keyword" and
"identifier" into one. Correct minimization here is *tag-aware*: start the
partition with one block per token tag (plus non-accepting), not just
`{F} | {Q\F}`. That's a real refinement a production lexer generator does; this
module leaves the DFA un-minimized to keep the focus on the union-and-tag
construction.

---

## The scan

### `tokenize` records `(lastPos, lastRule)`, backs up to it — (a)
Exactly Module 15's maximal munch, now over the *combined* DFA. Run until the
DFA can't advance (stuck, or a character outside the alphabet); the last
accepting `(position, rule)` is the token. `pos` then jumps to `lastPos`.

### A stuck position with no accept → `LexicalError(pos, char)` — (c)
A typed exception carrying the exact position and offending character — CSE 340
grades on precise error reporting. `pos` is a source-character index, so it's
right even after skipped whitespace.

### `%skip` tokens advance the cursor but aren't added to the output — (a)
They still went through the DFA and consumed characters; they're just filtered
from the returned list. `countSkipped` re-scans to report how many, for the demo.

### "epsilon IS NOOOOOT A TOKEN" — checked at load — (b), CSE 340's own rule
If any pattern's Thompson NFA accepts the empty string
(`εClosure({start}) ∩ accept ≠ ∅`), the spec is rejected. A nullable token
(`a?`, `(a|b)*`, `()`) would "match" length 0 at every position and the scanner
would spin forever. CSE 340 emits this exact error; we carry the phrase.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Data-driven spec, file order = priority | (b) | hard-coded rules — defeats the project |
| 2 | `%skip` for trivia | (c) | drop whitespace in the regex — can't, no escaping; or a special WS phase — less uniform |
| 3 | `.` as whitespace in the toy language | (c) | add escaping to Module 3 — scope creep |
| 4 | Union NFAs + tag + determinize | (a) | run each pattern separately (Module 15) — O(rules × input) |
| 5 | DFA state token = min tag in subset | (a) | track priority separately — the DFA already knows |
| 6 | Combined DFA NOT minimized | (c) | naive minimize — merges keyword/identifier; tag-aware minimize is out of scope |
| 7 | Typed `LexicalError` with position | (c) | a plain exception — CSE 340 grades error positions |
| 8 | Reject nullable patterns at load | (b) | catch the infinite loop at scan time — too late, and CSE 340 wants the load-time error |

---

## What We Proved

1. **A complete lexer is built from a spec file.** `fixtures/mini.spec` — 8
   `NAME regex` lines plus one `%skip` — compiles to a single 43-state DFA and
   tokenises arbitrary input.

2. **Maximal munch and priority happen in one pass.** `let.x=12.in.x` →
   `KW_LET ID ASSIGN NUM KW_IN ID`; every `let` in `let.let.in` is the keyword
   (priority), while `inlet` is one identifier (munch beats priority);
   `x<=99` munches `<=` as `LE`, not `<` then `=`.

3. **Whitespace is skipped, positions stay honest.** In `x.in` the `in` token
   reports position 2 — the skipped `.` still occupied position 1.

4. **Lexical errors carry an exact column.** `x=q` throws
   `LexicalError` at position 2, where `q` (not in the alphabet) sits.

5. **A nullable token is rejected before scanning.** `MAYBE = (a|b)*` in
   `fixtures/bad.spec` is refused at load with *"epsilon IS NOOOOOT A TOKEN"* —
   the check `εClosure({start}) ∩ accept ≠ ∅` on the pattern's NFA.
