# Module 05 — Decisions

Choices in `ParseTree.java` / `parsetree.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The tree

### A node is `(symbol, children, production)` — (c)
`symbol` is the grammar symbol. `children == null` means "not expanded yet";
`children == []` means "expanded by an epsilon production." `production` is the
rule that expanded this node — each node remembers its own, so reading a
derivation off the tree needs no external lookup table.

### Terminals and epsilon are both "leaves," distinguished by `children` — (c)
A terminal node stays `children == null` forever (it's never expanded). An
epsilon node gets `children == []`. `terminalYield` walks the tree: an
unexpanded node contributes its symbol; an expanded node with no children
contributes nothing. That one rule handles both leaf kinds.

### The tree is built from explicit production-index choices — (c)
`build(g, choices, leftmost)` applies `choices[0]`, `choices[1]`, … each to the
leftmost (or rightmost) unexpanded nonterminal. Same reasoning as Module 4:
a fixed script makes the demo deterministic and hand-checkable. A wrong choice
(production lhs ≠ frontier nonterminal) or a short list (tree left incomplete)
is a thrown error.

### `build` only needs ONE order — (a), essentially
You build the tree with, say, the leftmost choices. Then you extract *both* the
leftmost and the rightmost derivation *from that finished tree*. The tree
doesn't record an order, so there's nothing to build twice. This is the
module's whole point expressed as code structure.

---

## Reading a derivation off a finished tree

### `derivation(g, tree, leftmost)` replays the tree's expansions in a chosen order — (a)
Keep a list of "frontier nodes" starting `[root]`. Repeatedly: find the leftmost
(or rightmost) frontier node that is an *expanded nonterminal*, splice in its
children, record `(new symbol sequence, that node's production)`. Stop when no
expanded nonterminal remains on the frontier. The sequence of symbol lists is
the sequence of sentential forms.

### "Expanded nonterminal" is the pick condition, not "nonterminal" — (a)
Every nonterminal node in a *finished* tree has been expanded, so this is
equivalent — but stating it as "expanded" makes `derivation` also correct on
partial trees and makes the frontier logic identical to `build`'s.

### The production multiset is collected by a plain pre-order walk — (c)
`productionMultiset` walks the whole tree gathering `node.production`, sorts by
production index, and renders compactly (`E->E+T`). Sorting by index is what
makes "leftmost's multiset == rightmost's multiset" a string equality the demo
and tests can assert directly — the *order* the productions were applied is
exactly what we're claiming is irrelevant, so we sort it away.

---

## Rendering

### ASCII tree: `+- ` branch, `|  ` / `   ` continuation — (c)
Not Unicode box-drawing (`├─ └─ │`). Consistent with every other module's
ASCII-only program output — Windows console encoding mangles box characters and
it would break golden-file byte-parity. `+- ` for every child;
the continuation prefix for a child's subtree is `|  ` unless the child is the
last, in which case `   `.

### `render()` strips one trailing newline — (c)
So the tree can be `append`ed followed by an explicit `\n\n` without a blank
line creeping in. Java `stripTrailing()`, JS `.replace(/\s+$/, "")`.

---

## The example grammar

### An UNambiguous expression grammar — (c)
```
E -> E + T | T        T -> T * F | F        F -> ( E ) | id
```
Precedence (`*` over `+`) and associativity (left) are baked into the shape:
`+` lives at the `E` level, `*` at the `T` level, so `id + id * id` has exactly
one tree. The *ambiguous* version (`E -> E + E | E * E | id`) is Module 6's
subject — here we need a grammar with one tree so "the tree is prior to the
derivation order" isn't muddied by "which tree."

### The grammar is left-recursive, and that's fine — (a)
`E -> E + T` would loop a top-down parser forever (Module 16). But Module 5
*derives* — it applies productions the caller chose — it doesn't parse. Left
recursion is only a problem for the parsing *strategy*, not for the grammar or
its trees. Stating this now defuses a question that would otherwise nag from
here to Module 16.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Node = (symbol, children, production) | (c) | store the order too — it's exactly what we're proving irrelevant |
| 2 | `children` null vs [] distinguishes terminal from epsilon leaf | (c) | a separate `kind` enum — one field already carries it |
| 3 | Build from one order, extract both | (a) | build twice — the tree has no order to build |
| 4 | Multiset sorted by production index | (c) | keep application order — then it wouldn't be a *multiset* comparison |
| 5 | ASCII tree rendering | (c) | Unicode box chars — breaks console + golden parity |
| 6 | Unambiguous expression grammar | (c) | ambiguous one — confounds "the tree" with "which tree" (Module 6) |
| 7 | Left-recursive grammar left as-is | (a) | rewrite it — pointless here; deriving doesn't recurse infinitely |

---

## What We Proved

1. **One parse tree, two canonical derivations.** From the single tree for
   `id + id * id`, `derivation(g, tree, leftmost)` and
   `derivation(g, tree, rightmost)` produce two different sequences of sentential
   forms — differing from step 2 (`T + T` vs `E + T * F`).

2. **Same productions, different order.** Both derivations use the multiset
   `[E->E+T, E->T, T->T*F, T->F, T->F, F->id, F->id, F->id]` — identical. The
   only thing that changed is *when* each was applied.

3. **The tree determines the string.** `terminalYield()` reads `id + id * id`
   straight off the leaves, left to right.

4. **When there's no choice, the orders coincide.** For `a S b | ε`, every
   sentential form has at most one nonterminal, so leftmost and rightmost
   derivations of `aabb` are byte-for-byte identical. The orders only diverge
   when a sentential form contains two or more nonterminals — i.e. when there's
   an actual decision about which to expand.
