# Module 17 — Decisions

Choices in `Recovery.java` / `recovery.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## Recover, don't throw

### The error becomes data, not an exception — (c)
Module 16's `tokenize` throws `LexicalError` at the first bad character.
`Recovery.tokenize` instead appends a `LexError(from, to, text)` to a list and
keeps scanning. The caller gets *(tokens, errors)* — every error in one pass.
This is the difference between "compile failed: line 4" and "6 errors, all
listed."

### An `ERROR` token is emitted in the stream too — (c)
The garbage span becomes an `ERROR` token at its start position, so the token
stream stays continuous (every source character is covered by some token) and a
downstream parser can choose to skip `ERROR` tokens or report on them. The
separate `errors` list is the human-facing diagnostic.

---

## The two strategies

### `PANIC_ONE` — skip one character — (c)
The minimal recovery: consume the one offending character as an `ERROR` token,
resume. Simple, predictable, and it guarantees progress (always +1). Downside:
a run of `n` garbage characters produces `n` errors, which is noisy.

### `PANIC_TO_SYNC` — skip to a plausible token start — (b), classic panic mode
"Panic-mode recovery": on an error, discard input until you reach a token you
can trust to realign on. Here the *synchronizing set* is "characters that (1)
are in the alphabet and (2) the combined DFA can actually begin a token on"
(`longestToken(lex, input, i)[0] > i`). One `ERROR` token spans the whole run.
Fewer, wider errors. A real lexer's sync set is usually whitespace, `;`, `}`,
`)` — statement/block boundaries — chosen so recovery lands somewhere the parser
can resume.

### The sync check reuses `longestToken` — (c)
"Can a token start here?" is exactly "does the maximal-munch match at this
position have positive length?" — the same function the scanner already uses. No
separate "first character of any pattern" set to maintain.

### `errEnd` starts at `pos + 1` — progress is guaranteed — (a)
Even if the very next character is also a sync point, the bad character at `pos`
is always consumed. Without this the scanner could sit at the same position
forever.

---

## Not in scope here

### Line-level recovery — (c)
"Skip to the end of the line" is a common strategy, but the toy language has no
newlines. It's the same shape: sync set = `{newline}`.

### Insertion / substitution repair — (c)
Smarter recovery guesses what the programmer *meant* — insert a missing quote,
swap a typo'd character. That's least-cost repair (Appendix X9 territory), well
beyond a lexer.

### Cascading-error suppression — (c)
Real compilers stop reporting after ~20 errors, or suppress errors within
`k` characters of a previous one, because recovery often produces a burst of
spurious follow-on errors. Worth knowing; not implemented.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Errors as a returned list, not exceptions | (c) | throw — hides every error after the first |
| 2 | Emit an `ERROR` token spanning garbage | (c) | drop the garbage silently — parser loses source coverage |
| 3 | `PANIC_ONE` — skip one char | (c) | skip a fixed `k` — arbitrary |
| 4 | `PANIC_TO_SYNC` — skip to a token start | (b) | skip to whitespace only — misses `@x` where `x` is fine |
| 5 | Sync check = `longestToken(...)[0] > i` | (c) | maintain a separate first-char set — duplicate state |
| 6 | `errEnd` starts at `pos + 1` | (a) | start at `pos` — infinite loop |

---

## What We Proved

1. **One scan finds every lexical error.** `@x@y@` under `PANIC_ONE` reports
   3 errors, at positions 0, 2, 4. Module 16's `tokenize` throws at position 0
   and you never learn about the other two.

2. **`PANIC_ONE` gives one error per bad character.** `x@@@#y` → four `ERROR`
   tokens, four diagnostics.

3. **`PANIC_TO_SYNC` gives one error per garbage run.** The same `x@@@#y` →
   `ID ERROR ID`, the `ERROR` token's text is `"@@@#"`, one diagnostic spanning
   positions 1–4. Fewer, more useful messages.

4. **Recovery keeps the good tokens.** `let.x=@=9` → `KW_LET ID ASSIGN ERROR
   ASSIGN NUM` — the `@` is isolated and the `= 9` after it still tokenise
   correctly. A clean input produces zero errors and zero `ERROR` tokens.
