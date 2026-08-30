# Module 06 — Decisions

Choices in `Ambiguity.java` / `ambiguity.mjs` and `Main`, sorted into
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## Finding all the trees

### Enumerate leftmost derivations, not "trees" directly — (a)
A parse tree is in bijection with its leftmost derivation (Module 5). So "find
every tree for `w`" = "find every leftmost derivation that yields `w`," and the
second is a concrete search over sentential forms. `allTrees` builds a
`ParseTree` from each and de-dupes by rendered form (defensive — with no
duplicate productions in the grammar, distinct derivations already give distinct
trees).

### Breadth-first, with two prunes — (a)
- **Terminal-count bound.** Terminals are never a left-hand side, so a form with
  more terminals than `|w|` can't reach `w`. (Same fact as Module 4.)
- **Terminal-prefix match.** In a leftmost derivation, everything to the left of
  the leftmost nonterminal is *finished* — it will never change. So that prefix
  must already equal the corresponding prefix of `w`, or the branch is dead.
  This is the prune that makes the search fast: it kills a wrong first token
  immediately instead of after generating the whole string.

### `MAX_STATES = 1_000_000` safety valve — (c)
Our grammars have no unit or ε cycles, so the search always terminates well
below this. The cap only matters if someone feeds in a cyclic grammar — which
Modules 16 and 23 exist to fix.

### `smallestAmbiguousString` walks `enumerate` in canonical order — (c)
"Smallest" = first in (length, lexicographic) order (Module 4's enumeration
order). For each string, run `allTrees`; return the first with ≥ 2. It's a
witness generator: a single concrete string proving the grammar is ambiguous.

### Ambiguity is undecidable in general — stated, not hidden — (a)
There is no algorithm that decides, for an arbitrary CFG, whether it is
ambiguous. `smallestAmbiguousString` only searches up to `maxLen`; a `null`
result means "no witness *this short*," never "unambiguous." The tutorial says
this plainly.

---

## Disambiguation

### We *exhibit* the standard rewrites, we don't *compute* them — (c)
Two grammar files ship: `expr-ambiguous` and `expr-unambiguous`. The module's
job is to show (1) the first is ambiguous, (2) the second isn't, (3) they
generate the same language. Automatically transforming an arbitrary ambiguous
grammar into an equivalent unambiguous one is not possible in general (it's tied
to the undecidability above), so there's nothing to automate here.

### The two rewrites, named — (b), these are the universal conventions
- **Precedence** → one nonterminal tier per precedence level. `+` handled at
  `E`, `*` at `T`, atoms at `F`. A tighter operator lives at a lower tier, so
  it nests deeper in the tree, so it groups first.
- **Associativity** → recursion side. `E -> E + T` (left-recursive) makes `+`
  left-associative: `a + b + c` = `(a + b) + c`. `E -> T + E` would make it
  right-associative. This is exactly what Module 20 will formalise.

### Language equivalence is checked by bounded enumeration — (c)
`Derivation.enumerate(amb, 7)` vs `enumerate(unamb, 7)` — both produce 60
strings, identical lists. This is *evidence*, not proof (a difference could hide
past length 7). Exact CFL equivalence is undecidable; the honest tool is
"compare a generous bounded sample," which is what we do. Module 14 gives the
*regular*-language version, which *is* decidable.

### `evalExpr` / `grouping` are grammar-specific helpers in `Main` — (c)
They only understand the shape "single child → recurse; `X op Y` → combine;
`( X )` → recurse," which fits both the ambiguous and the disambiguated
expression grammar. They exist to make ambiguity *concrete*: the two trees for
`id + id * id` don't just look different, they compute 6 and 8.

---

## The dangling-else fixture

### `S -> if x then S | if x then S else S | a | b` — (c)
The condition is the literal token `x` and the branches are literal `a` / `b`,
so the grammar has no sub-language to distract from the structural point. The
string `if x then if x then a else b` has two trees; the difference is which
`if` the single `else` belongs to.

### "else binds to OUTER iff the root uses the 6-symbol production" — (c)
Production 1 (`if x then S else S`) expands to 6 children. If the *root* `S` used
it, the `else` is at the top level → binds to the outer `if`. If the root used
production 0 (`if x then S`, 4 children), the `else` is buried in the nested `S`
→ binds to the inner `if`. One `children.size()` check labels each tree.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Enumerate leftmost derivations | (a) | enumerate trees directly — same thing, harder to bound |
| 2 | Terminal-prefix prune | (a) | only the count bound — orders of magnitude slower |
| 3 | Ship two grammars, don't transform | (c) | write a disambiguator — impossible in general |
| 4 | Language equivalence by bounded enumeration | (c) | claim exact equivalence — undecidable for CFLs |
| 5 | Grammar-specific `evalExpr` | (c) | no evaluator — ambiguity stays abstract |
| 6 | Dangling-else with literal `x`/`a`/`b` | (c) | a real expression grammar for the condition — noise |
| 7 | Label else-binding by root production arity | (c) | walk the tree looking for the else — the root check is exact and one line |

---

## What We Proved

1. **The textbook expression grammar is ambiguous.** `id + id * id` has exactly
   two parse trees under `E -> E + E | E * E | ( E ) | id`. They are not
   cosmetic: with `id = 2` they evaluate to **6** and **8**.

2. **Associativity is ambiguity too.** `id + id + id` also has two trees —
   `(id + id) + id` and `id + (id + id)`. Here both give 6 (addition is
   associative), but for `-` or `/` they wouldn't, and the parser still has to
   pick one.

3. **Disambiguation works and preserves the language.** The tiered grammar
   `E/T/F` gives every string exactly one tree, forces `*` to bind tighter than
   `+` and `+` to associate left — and generates the identical 60-string
   language (up to length 7) as the ambiguous one.

4. **Some ambiguities aren't about operators.** The dangling `else` —
   `if x then if x then a else b` — has two trees purely from nested optional
   structure. Real languages fix it by fiat ("`else` matches the nearest
   unmatched `then`"), which Module 35 implements as a shift/reduce preference.

5. **Ambiguity detection is bounded, not decided.** `smallestAmbiguousString`
   finds a witness if one is short enough. It can never certify "unambiguous" —
   that question has no algorithm.
