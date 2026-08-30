# Module 26 — Decisions

Choices in `TableParser.java` / `tableparser.mjs`, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The algorithm

### Explicit stack of (symbol, tree-node) pairs — (b)
The canonical table-driven predictive parser (Dragon Book §4.4.4). We pair each
stack symbol with the parse-tree node it will fill, so the tree is built in the
same pass with no second traversal. Stack starts `[$ , start]`; input ends with
`$`.

### Push the RHS reversed — (a)
`A -> X1 X2 X3`: push `X3`, `X2`, `X1`. The stack must present `X1` next, and a
stack is LIFO. This is what keeps the stack equal to "the remaining sentential
form, leftmost symbol on top" at every step.

### Three cases, in this order — (a)
1. top is `$`: accept iff lookahead is `$`, else "input left over".
2. top is a terminal: must equal the lookahead — pop and advance, or error.
3. top is a nonterminal: `M[top][lookahead]` — blank or conflict is an error,
   otherwise expand.

### An empty RHS still gets an `epsilon` tree node — (c)
So the parse tree records that `A -> epsilon` was applied (visible as an
`epsilon` leaf), matching Module 19's concrete trees. The stack gets nothing
pushed.

---

## The proof that it equals recursive descent

### Emit the production sequence and replay it as a leftmost derivation — (c)
`parse` records every production it applies. `replay` starts from the start
symbol and applies them to the leftmost nonterminal, one at a time; the final
sentential form must equal the input token list. This makes "the stack is the
leftmost derivation" (Module 19's claim) executable for the table-driven parser
too — same production sequence, produced by lookup instead of recursion.

### Token streams are terminal names, not lexed source — (c)
Fixtures give inputs as space-separated terminal symbols (`id + id * id`). This
module is about the *parser*, so it skips lexing entirely; the terminal name
*is* the token. A real front end would put Module 9–18's lexer in front.

---

## Errors

### Report the failing (stack top, lookahead, position); keep the partial trace and productions — (c)
On a blank cell or a terminal mismatch, `parse` returns `ok=false` with the
position and the productions applied so far. No recovery yet — the parse stops.
Module 27 adds panic-mode recovery so it can continue.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | (symbol, node) pairs on the stack | (b) | parse tree in a second pass |
| 2 | Push RHS reversed | (a) | forward — wrong symbol next |
| 3 | `epsilon` leaf for empty RHS | (c) | omit — tree loses which rule fired |
| 4 | Replay productions to prove leftmost | (c) | assert the claim in prose only |
| 5 | Inputs are terminal names, no lexer | (c) | wire in the Part II lexer — off topic |
| 6 | Stop on first error, keep partial state | (c) | throw and lose the trace |

---

## What We Proved

1. **The table plus a stack parses the language.** `id + id * id`,
   `( id + num ) * id`, `num` all reach `ACCEPT`.

2. **It produces the same leftmost derivation as recursive descent.** The
   recorded production sequence, replayed leftmost, reproduces the input exactly
   (`equals input? true`) — and it's the identical sequence Module 19's call
   stack fires.

3. **The parse tree comes for free.** Built during the walk, same shape as
   Module 19's concrete trees, `epsilon` leaves and all.

4. **Errors are located.** `id +` → blank `M[T][$]` at position 2; `id id` →
   blank `M[Tp][id]` at position 1; `( id + id` → expected `)` at position 4.

5. **No recursion, no per-grammar code.** The same loop parses any LL(1)
   grammar; only the table changes.
