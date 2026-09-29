# Module 25 — Decisions

Choices in `LL1Table.java` / `ll1table.mjs`, in three buckets: **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## Building the table

### `M[A][t] = p` for every `t ∈ PREDICT(p)` — (b)
The canonical construction (Dragon Book §4.4.3). `PREDICT` already folds in
`FOLLOW` for nullable productions (Module 20), so the single loop handles both
the FIRST cells and the FOLLOW cells. Nothing grammar-specific.

### A cell holds a LIST of productions, not one — (c)
So a conflict is visible as `cell.size() > 1` rather than a silent overwrite.
The build never discards a competing production; the conflict report shows both.
An LL(1) grammar simply has every list at length ≤ 1.

### Columns are the terminals plus `$` — (a)
The lookahead is always a terminal or end-of-input. Nonterminals never index the
table. Column order = first appearance of each terminal in a right-hand side,
then `$` — deterministic.

### Blank cell = syntax error, not `epsilon` — (a)
An empty `M[A][t]` means "no production of `A` can begin with (or be followed,
if nullable, by) `t`" — the input is malformed at that point. It is distinct
from `M[A][t] = (A -> epsilon)`, which is a real, deliberate move.

---

## Rendering

### Cells show the production INDEX, with a numbered legend — (c)
`M[E][id] = 0` is readable in a grid; `M[E][id] = E->TEp` is not (and
`S->idassignEsemi` is unreadable). The legend prints the full productions once;
the grid stays scannable. `!` marks a conflict, `.` a blank.

### The parse trace is included as a Module 26 preview — (c)
`Main` ends by running the finished `expr` table over `id + id * id` with an
explicit stack. It shows the table is *executable*, not just a static artifact —
and motivates Module 26, which is exactly this loop, formalised.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Fill from `PREDICT` (FIRST + FOLLOW-if-nullable) | (b) | FIRST only — misses nullable cells |
| 2 | Cell = list of productions | (c) | single slot, overwrite — hides conflicts |
| 3 | Columns = terminals + `$` | (a) | include nonterminals — meaningless |
| 4 | Blank ≠ `epsilon` | (a) | treat blank as "take epsilon" — accepts bad input |
| 5 | Grid shows production index + legend | (c) | inline production text — unreadable |
| 6 | Include a stack-driven parse trace | (c) | leave the table inert |

---

## What We Proved

1. **The expression grammar's table is single-valued** — every cell has ≤ 1
   production, so it is LL(1) and a table-driven parser exists.

2. **FOLLOW cells are real.** `M[Ep][)]` and `M[Ep][$]` hold `Ep -> epsilon`,
   placed there by `FOLLOW(Ep)`, not `FIRST`.

3. **The dangling else is one double-booked cell.** `M[Sp][else]` wants both
   `Sp -> else S` and `Sp -> epsilon`. Left factoring (Module 24) removed the
   shared prefix but not this.

4. **Blank cells are syntax errors.** `M[E][+]` is empty — a `+` where an
   expression is expected is rejected immediately.

5. **The table drives a parse.** The trace expands `id + id * id` to `ACCEPT`
   using only stack operations and table lookups.
