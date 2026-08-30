# Module 27 — Decisions

Choices in `RecoveringParser.java` / `recoveringparser.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The recovery strategy

### Two mechanisms, chosen by which thing failed — (b)
- **Terminal on top ≠ lookahead** → **phrase-level**: assume the expected
  terminal was there, pop it, don't consume input. This turns "missing `)`"
  into one error and a completed parse instead of a cascade.
- **`M[A][t]` blank (or conflicted)** → **panic mode** with a synchronizing
  set. Both are Dragon Book §4.4.5.

### The synchronizing set is `FOLLOW(A)` — (b)
When the parser can't expand `A` on the current token, the question is "can I
skip `A` and pick up where `A` would have ended?" — and the tokens that can
follow `A` are exactly `FOLLOW(A)`. If the lookahead is in `FOLLOW(A)` (or `$`),
pop `A`. Otherwise the token is genuinely stray: discard it and try `A` again.

Some texts also fold `FIRST(A)` into the sync set (to resume *without*
discarding when the lookahead could still start `A`). For an LL(1) table that
case can't produce a blank cell, so we don't need it here; the Project 2 tool
(Module 28) can add it.

### Guaranteed termination — (a)
Every recovery branch does exactly one of: **pop the stack** (phrase-level
insert, or panic-mode sync) or **advance the input** (panic-mode discard). The
stack and the input are both finite and only move one way, so the loop must
end. A `guard` counter is a belt-and-braces cap, never expected to trip.

---

## What it reports

### An ordered error list with positions, plus a full trace — (c)
`errors` is `[(position, message)]` in the order they were hit — the human
diagnostic. `trace` is every step including recovery actions — the teaching
artifact. `accepted` says whether it reached end of input; `clean()` means
accepted with zero errors.

### Messages name the recovery action, not just "syntax error" — (c)
"no rule for `T` on `')'`; skipping `T` (lookahead in FOLLOW)" tells you both
what was wrong and what the parser did about it — so the later cascade (if any)
is interpretable.

### No parse tree this module — (c)
Module 26 built the tree; here the focus is the recovery control flow, and a
tree with holes in it would distract. The tree + recovery combination is
straightforward to assemble and is left to Project 2.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Phrase-level for terminal mismatch | (b) | panic mode for everything — worse messages |
| 2 | Panic mode for blank cells | (b) | stop at first error (Module 26 behaviour) |
| 3 | Sync set = `FOLLOW(A)` | (b) | `FIRST∪FOLLOW` — unnecessary for an LL(1) table |
| 4 | Every branch pops or advances | (a) | risk a no-progress loop |
| 5 | Ordered error list + full trace | (c) | just a count |
| 6 | No parse tree here | (c) | build a tree with holes |

---

## What We Proved

1. **One run finds every error.** Each fixture recovers to end of input and
   reports its error(s) with positions — the parser never gives up early.

2. **Panic mode, both branches.**
   - `id + * id` → the `*` isn't in `FOLLOW(T)`, so it's **discarded**.
   - `( id + )` → the `)` **is** in `FOLLOW(T)`, so `T` is **skipped**.

3. **Phrase-level insertion works.** `( id + id` (missing close paren) →
   "inserted missing `)`", parse completes.

4. **It always terminates.** Even `* * * *` returns with an error list; no
   branch fails to make progress.

5. **A clean input stays clean.** `id + id * id` → accepted, zero errors — the
   recovery machinery is inert when nothing is wrong.
