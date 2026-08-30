# Module 09 — Decisions

Choices in `Buffer.java` / `buffer.mjs` and `Scanner`, sorted into
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The scheme

### Two halves, not one growable buffer — (b), the Dragon Book contract
§3.2 of Aho/Lam/Sethi/Ullman. Real input arrives in blocks; you fill one half,
scan into it, and when `forward` reaches the boundary you fill the *other* half
and keep going. The half holding `lexemeBegin` stays intact until the lexeme is
emitted. A single growable buffer would work in memory but wouldn't model the
"can't hold the whole file, must retract" problem the scheme exists to solve.

### The sentinel is `-2`; true EOF is `-1`; characters are `int` — (a)
The sentinel must be a value that *cannot appear in the input*. Java `char` has
no spare value, so the buffer stores `int`s and `advance()` returns `int` —
exactly like `java.io.Reader.read()` returning `-1` for EOF. `-2` for the
half-boundary sentinel, `-1` for real end. JS uses an `Int32Array` for the same
reason.

### Why a sentinel at all — one compare instead of two — (b)
Without it, the inner loop is `if (forward == bufferEnd) { reload } else if
(forward == trueEnd) { stop } else { c = buf[forward++] }` — two bounds checks
per character. With a sentinel written into the slot *after* the real data, the
loop is `c = data[forward]; if (c == SENTINEL) { reload-or-stop }` — **one**
compare, and it's the same compare whether we hit a buffer boundary or the end
of input. That's the entire point of the technique; the instrumentation
(`forwardReads`) shows ~1 per character.

### `EOF` written inline when a half isn't full; `SENTINEL` when it is — (a)
If `fillHalf` copies fewer than `halfSize` characters, the source is exhausted,
so it writes `EOF` right after the last real character — no boundary to cross.
If it copies a full half, it writes `SENTINEL` at the dedicated slot. When the
source length is an exact multiple of `halfSize`, the last full half gets a
`SENTINEL`, and the *next* `crossBoundary` does a 0-character fill that writes
`EOF` — the `+1` load in the classic cost formula.

### Lexeme must fit within one half — (b), and stated as a limit
The scheme cannot retract across the halfway boundary, because the half you'd
retract into may already have been overwritten. `retract()` throws if asked to.
Real lexers set `halfSize` generously (4 KB is typical) so no realistic token —
identifier, string literal, comment — overflows it. `lexeme()` *can* stitch a
lexeme that spans the boundary, as long as it fits in one half's worth of
characters.

---

## The API

### `peek` / `advance` / `mark` / `retract` / `lexeme` — (c)
`peek()` — current char, no move; skips sentinels transparently.
`advance()` — current char then move; returns `EOF` at the end and stays there.
`mark()` — `lexemeBegin = forward`, i.e. "the lexeme starts here."
`retract(n)` — `forward -= n`, O(1).
`lexeme()` — the text from `lexemeBegin` to `forward`, stitched across a split.
This is the same five-verb cursor as the prerequisites page, plus the block
management underneath.

### `advance()` at EOF returns `EOF` and does **not** move — (c)
Idempotent end-of-input. A scanner loop `while ((c = advance()) != EOF)` then
terminates cleanly, and calling `advance()` again is harmless.

### Instrumentation counters are package-private, read by `Main` directly — (c)
`bufferLoads`, `forwardReads` exist only to let the tutorial show the O(1)
claim with real numbers. No getters — `Main` and the tests are in the same
package. A production buffer wouldn't carry them.

---

## The demo scanner

### A four-token toy scanner, built on `Buffer` — (c)
`WORD [a-z]+`, `NUM [0-9]+`, `FLOAT [0-9]+ '.' [0-9]+`, `PLUS '+'`. Its only job
is to exercise the buffer. The `FLOAT` rule is deliberately the one that needs
`retract`: after reading `12.` it must look one character further, and if that
isn't a digit (`12.go`), the `.` belongs to the next token and `forward`
retracts one. That's the maximal-munch lookahead of Module 15 in miniature.

### `retract` count is threaded through a `int[] retracts` / `[0]` array — (c)
Java has no out-parameters; a one-element array is the standard workaround.
The JS twin uses the same shape for byte-identical output.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Two halves, filled on demand | (b) | one growable buffer — doesn't model the real constraint |
| 2 | Sentinel `-2`, EOF `-1`, `int` chars | (a) | a `char` sentinel — no spare value exists |
| 3 | One `c == SENTINEL` compare per char | (b) | `forward == end` bounds check every char — the thing being avoided |
| 4 | Lexeme ≤ one half; `retract` throws otherwise | (b) | handle arbitrarily long lexemes — the scheme structurally can't |
| 5 | `advance()` idempotent at EOF | (c) | throw at EOF — makes the scan loop uglier |
| 6 | Public counters for instrumentation | (c) | no measurement — then the O(1) claim is just an assertion |
| 7 | FLOAT rule to force a retract | (c) | a scanner with no retract — nothing to demonstrate |

---

## What We Proved

1. **A scanner can retract arbitrary (bounded) lookahead at O(1).** `retract(1)`
   is `forward--`. The `12.go` case: read `1 2 .`, peek `g`, retract the `.`,
   emit `NUM "12"` — and the `.` is scanned fresh as its own token.

2. **Buffer loads grow like N / halfSize, not like N.** 100 characters at
   halfSize 10 → 11 loads; 1000 characters → 101 loads. Ten times the input,
   ten times the loads — amortized O(1) per character.

3. **The sentinel collapses two checks into one.** `forwardReads` ≈ characters
   consumed (40 for 39 chars). The scanner never separately tests "am I at the
   buffer end?" and "am I at the input end?" — one `c == SENTINEL` answers both.

4. **A lexeme that straddles the halfway split is recovered intact.**
   `"brownish"` begins in half 1, a reload swaps half 0, and `lexeme()` stitches
   the two fragments back into `"brownish"` — as long as it fits in one half.
