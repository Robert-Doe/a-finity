# Module 17 — Decisions

Choices in `Recovery.java` / `recovery.mjs`, sorted into **(a) forced**,
**(b) standard practice**, **(c) our convention**.

---

## Recover, don't throw

### The error becomes data, not an exception — (c)
Module 16's `scan` throws `ScanError` at the first bad character.
`scanRecovering` instead appends a `LexError(line, col, text)` to a list and
keeps scanning. The caller gets *(tokens, errors)*: every error in one pass.
This is the difference between "compile failed: line 4" and "6 errors, all
listed."

### An `ERROR` token is emitted in the stream too — (c)
The bad span becomes an `ERROR` token at its start, so the token stream stays
continuous (every source character is covered by some token or by discarded
text) and a downstream parser can skip `ERROR` tokens or report on them. The
separate `errors` list is the human-facing diagnostic.

### Every error carries `line:col` — (b)
Module 16 already tracks positions this way, and a diagnostic without a
location is useless in a multi-line file.

---

## The two strategies

### `SKIP_ONE` — skip one character — (c)
The minimal recovery: consume the one offending character as an `ERROR` token,
resume. Simple, predictable, and it guarantees progress (always +1). Downside:
a run of `n` bad characters produces `n` errors, which is noisy.

### `SKIP_TO_RESTART` — skip until a token can start — (b), panic mode
On an error, discard input until you reach a position you can trust to realign
on. Here the *synchronizing set* is "any position where the scanner finds a
match" (`longestMatch(src, i) != null`). One `ERROR` token spans the whole run.
Fewer, wider errors. A parser's sync set is usually `;`, `}`, `)`: statement
and block boundaries, chosen so recovery lands somewhere it can resume.

### The sync check reuses `longestMatch` — (c)
"Can a token start here?" is exactly "does the longest match at this position
have positive length?", the same function the scanner already uses. No separate
"first character of any pattern" set to maintain.

### `end` starts at `pos + 1`: progress is guaranteed — (a)
Even if the very next character could start a token, the bad character at
`pos` is always consumed. Without this the scanner could sit at the same
position forever.

### Discarded text is never an error — (a)
A `//` comment containing `@#$?` matches the `COMMENT` rule as a whole, so its
contents never reach the recovery path.

---

## Not in scope here

### Insertion / substitution repair — (c)
Smarter recovery guesses what the programmer *meant*: insert a missing quote,
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
| 1 | Errors as a returned list, not exceptions | (c) | throw: hides every error after the first |
| 2 | Emit an `ERROR` token spanning the bad text | (c) | drop it silently: the parser loses source coverage |
| 3 | `line:col` on every error | (b) | a flat character offset |
| 4 | `SKIP_ONE`: skip one char | (c) | skip a fixed `k`: arbitrary |
| 5 | `SKIP_TO_RESTART`: skip to a token start | (b) | skip to whitespace only: misses `@x` where `x` is fine |
| 6 | Sync check = `longestMatch(src, i) != null` | (c) | a separate first-char set: duplicate state |
| 7 | `end` starts at `pos + 1` | (a) | start at `pos`: infinite loop |

---

## What We Proved

1. **One scan finds every lexical error.** `fixtures/typos.ajoda` has typos on
   lines 2, 3 and 5; one recovering scan reports all three with `line:col`.
   Module 16's `scan` stops at the first.

2. **`SKIP_ONE` gives one error per bad character.** `a @@@# b` → four `ERROR`
   tokens, four diagnostics.

3. **`SKIP_TO_RESTART` gives one error per bad run.** The same `a @@@# b` →
   `IDENT ERROR IDENT EOF`, the `ERROR` token's text is `"@@@#"`, one
   diagnostic at `1:3`.

4. **Recovery keeps the good tokens.** `let n: i64 = 5 @ 2;` →
   `LET IDENT COLON I64 ASSIGN INT_LIT ERROR INT_LIT SEMI EOF`. A clean input
   produces zero errors and zero `ERROR` tokens.
