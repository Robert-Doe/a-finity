# Module 08 — Decisions

Choices in `Ebnf.java` / `ebnf.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The EBNF dialect

### One small dialect, four operators — (c)
`?` optional, `*` zero-or-more, `+` one-or-more, `( )` grouping. Plus `|`,
juxtaposition, `'...'` / `"..."` terminals, `;` rule terminator, `//` comments.
Real EBNF/ISO-EBNF have more (comments as `(* *)`, `,` for concatenation,
`n * x` counts, exceptions `-`) but these four are what "EBNF adds to BNF" means
and what the desugaring has to handle.

### `=` and `;`, not `::=` — (b), matching common practice
Most EBNF-in-the-wild uses `name = ... ;`. BNF uses `::=` or `->`. The cheat-sheet
lays out all four dialects' choices side by side; the fixture format picks one.

### An unquoted name with no rule is a terminal — (c)
Same convention as the BNF grammar files (Module 4). `NUM` in `factor = '(' expr
')' | NUM` has no rule, so it's a terminal. The matcher's `Nonterminal` case
falls back to "match this literally" when `rules.get(name)` is null.

---

## Desugaring to BNF

### One fresh nonterminal per operator, named by kind — (c)
`x?` → `opt_N -> x | epsilon`. `x*` → `rep_N -> x rep_N | epsilon`. `x+` →
`plus_N -> x plus_N | x`. A parenthesised `( ... )` that isn't a bare atom →
`grp_N -> ...`. The prefix (`opt_`/`rep_`/`plus_`/`grp_`) makes the desugared
grammar self-documenting.

### `x+` desugars to `P -> x P | x`, NOT `x x*` — (c)
Both are correct. `P -> x P | x` (right-recursive, "one or more") is one rule;
`x x*` would be `x` followed by a fresh `rep_` rule — two nonterminals and an
extra level. The single-rule form is smaller and the tests pin it.

### `x*` is right-recursive (`R -> x R | ε`), not left (`R -> R x | ε`) — (c)
Right recursion so a top-down parser can use the result directly (left recursion
would need Module 16's transform). The language is identical either way; the
recursion side matters only to the *parsing method*, exactly as in Module 6.

### Reserve-then-fill so helper rules print in creation order — (c)
`symbol()` inserts `extra.put(name, "")` *before* recursing into sub-expressions,
then overwrites with the real rule text. Because the map preserves first-insertion
order, `rep_1` prints before the `grp_2` it spawned, which prints before
`grp_3` — the grammar reads top-down. Filling after the recursion (the first
version) printed `grp_3, grp_2, rep_1` — valid but backwards.

### Grouping creates a nonterminal only when it's not already a bare atom — (a)
`(x)` where `x` is a single terminal/nonterminal just returns `x` — the parser's
`atom` rule unwraps `'(' alt ')'` and returns the inner expression. A fresh
`grp_` rule appears only for a group that is a real `Seq` or `Alt`.

### The desugared grammar is emitted as **text**, then `Grammar.parse`d — (c)
Module 4's `Grammar` has a private constructor and a text loader. Rather than
add a public builder, the desugarer produces BNF source and hands it to the
existing, tested loader. One format, one parser.

---

## Verifying equivalence

### Two checks: exact matching + bounded enumeration — (c)
- **`accepts(ebnf, tokens)`** — an exact leftover-suffix matcher over the EBNF
  AST, structurally identical to Module 3's regex matcher (same `rest != pos`
  guard on `Star`). Decides one token list, no length bound.
- **Enumerate `L(BNF)` up to `maxLen`**, then assert every string is
  `accepts`-ed by the EBNF. This shows `L(BNF) ⊆ L(EBNF)` on a generous sample.
  Combined with the per-operator desugaring being meaning-preserving by
  construction, that's strong evidence of `L(BNF) = L(EBNF)`.

### It's evidence, not proof — (a)
Exact CFL equivalence is undecidable (Module 6). We check a bounded slice and a
sample of positives/negatives. The desugaring's correctness is really an
argument, not a computation — each rewrite is a standard identity.

---

## ISO-style rendering

### `{ }` = zero-or-more, `[ ]` = optional — (b)
This is ISO 14977 EBNF, a third real dialect. Rendering the same AST three ways
(as-written, ISO-style, desugared BNF) is the module's "these are all the same
grammar" point made visible. `isoRule` skips the outer parens on a top-level
`Alt` for readability.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Four operators: `? * + ( )` | (c) | full ISO-EBNF — more sugar, same desugaring lesson |
| 2 | `=` / `;` rule syntax | (b) | `::=` — the cheat-sheet covers all dialects anyway |
| 3 | Undefined name ⇒ terminal | (c) | require `%token` declarations — extra ceremony |
| 4 | One fresh nonterminal per operator, kind-prefixed name | (c) | numbered generic names — less readable output |
| 5 | `x+` → `P -> x P \| x` (one rule) | (c) | `x x*` — two nonterminals, taller tree |
| 6 | Right-recursive `x*` | (c) | left-recursive — needs Module 16 to parse |
| 7 | Reserve-then-fill helper emission | (c) | fill-after — helpers print in reverse spawn order |
| 8 | Emit BNF text, reuse `Grammar.parse` | (c) | add a public Grammar builder — new surface, same result |
| 9 | Equivalence by matcher + bounded enum | (a) | claim exact equivalence — undecidable |

---

## What We Proved

1. **EBNF's operators are pure sugar.** `expr = term (('+'|'-') term)*` desugars
   to seven plain BNF rules with `epsilon` and recursion — no `*`, `?`, `+`, or
   parentheses survive. `signed = '-'? digit+` desugars to four.

2. **The desugaring preserves the language.** Every one of the 220 strings that
   `L(BNF)` generates (up to length 7) is accepted by an exact matcher run
   against the *original EBNF*. Same for `signed`. Sample positives and
   negatives agree on both sides.

3. **BNF, EBNF, ISO-EBNF, and ABNF are one thing in four notations.** The same
   grammar renders as `term (('+'|'-') term)*` (this course),
   `term { ( '+' | '-' ) term }` (ISO), and nine `->`/`|` rules (BNF). The
   cheat-sheet maps every construct across all four.

4. **This is the same move as Module 3.** There, `+` and `?` desugared into
   `Union`/`Concat`/`Star` for regex. Here, `*`/`+`/`?`/`( )` desugar into
   recursion and `epsilon` for grammars. Sugar is sugar; the core is always
   small.
