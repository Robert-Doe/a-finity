# Module 23 — Decisions

Choices in `LeftRec.java` / `Language.java` and mirrors, in three buckets:
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## The transform

### Direct elimination is the standard textbook rewrite — (b)
`A -> A a1 | ... | A ak | b1 | ... | bm`  becomes
`A -> b1 A' | ... | bm A'` and `A' -> a1 A' | ... | ak A' | epsilon`.
This is Aho/Sethi/Ullman §4.3.3. It preserves the language (every string
`b (a...)*` is generated both ways) and moves the recursion to the right, where
a token is consumed before `A'` recurses — so recursive descent terminates.

### Indirect elimination is Paull's algorithm, with the grammar's own nonterminal order — (b)
Fix `A1..An`. For `i = 1..n`: for `j = 1..i-1`, replace `Ai -> Aj g` with
`Ai -> d g` for every `Aj -> d`; then eliminate direct left recursion in `Ai`.
After step `i`, no `Ai` production starts with `A1..Ai`. We use the order in
which nonterminals first appear as a left-hand side — deterministic, and it
matches how a reader would number them.

### Paull's substitution step is guarded — (c)
Textbook Paull substitutes *every* `Ai -> Aj g` with `j < i`. That's
language-preserving but bloats the grammar when there was no indirect left
recursion to expose (a rule like `elements -> value comma elements` gets
`value` inlined for nothing). We only substitute `Aj` into `Ai` when `Aj` can
reach `Ai` as a leftmost symbol — the only case where it turns indirect left
recursion into direct. Both fixtures behave identically; a grammar like JSON's
`elements` list stays readable.

### The generated grammar leans right — associativity moves elsewhere — (a)
`E -> E + T` (left-recursive) builds a left-leaning tree = left associativity.
`E -> T E'` builds a right-leaning one. The transform **cannot** keep the tree
shape — that shape is what made it left-recursive. Left associativity is
recovered when lowering to an AST (Module 39) or by writing `E'` as a loop
(Module 19 §04). The transform preserves the *language*, not the *parse trees*.

### We do not left-factor here — (c)
Paull's substitution step can create alternatives with shared prefixes. Left
factoring them is Module 24's job; this module stops once left recursion is
gone.

---

## Scope limits

### Nullable-prefix left recursion is out of scope — (c)
`A -> B A x`, `B -> epsilon` is technically left recursion (A can reach `A x`).
Neither fixture has this, `hasLeftRecursion` is a syntactic check that ignores
it, and the full treatment (compute nullable, treat nullable prefixes as
transparent) belongs with Module 21's machinery in the Project 2 tool
(Module 28).

### Generated primes are assumed already recursion-free — (a, true here)
`A'` productions are `alpha A'` and `epsilon`, where `alpha` is the part of a
rule *after* the leading `A`. `alpha` cannot start with `A` (we stripped it).
Pathological cases where `alpha` starts with a *later* nonterminal that loops
back through `A'` are not handled; they don't arise for LL-target grammars.

---

## The language check

### Equivalence is verified empirically, by bounded enumeration — (c)
A full proof that Paull preserves the language is a paper argument. Here we
generate every string of length ≤ 6 from both grammars (BFS over sentential
forms, leftmost expansion, prune when the terminal count exceeds the bound) and
assert the sets are equal. A divergence at length ≤ 6 would be a real bug; this
catches the mistakes people actually make (a dropped alternative, a missing
`epsilon`).

### The BFS has a hard budget — (a)
400,000 form-expansions and a form-length cap of `4·maxLen + 4`. A grammar that
*still* has left recursion would enumerate forever otherwise. For the fixtures
the search completes far under budget (70 and 20 strings).

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Standard direct-elimination rewrite | (b) | ad hoc rewrites — easy to change the language |
| 2 | Paull's algorithm for indirect | (b) | only handle direct — misses `A -> S d` cases |
| 3 | Nonterminal order = first-appearance order | (c) | arbitrary / alphabetical |
| 4 | Accept the right-leaning tree | (a) | impossible to avoid with top-down parsing |
| 5 | No left factoring here | (c) | do both — muddies two separate transforms |
| 6 | Empirical language equivalence, bounded | (c) | trust the transform / formal proof only |
| 7 | Hard budget on the enumerator | (a) | unbounded — hangs on a buggy transform |

---

## What We Proved

1. **The Module 19–22 grammar is derived, not invented.** `E -> E + T | T`,
   `T -> T * F | F` becomes exactly `E -> T E'`, `E' -> + T E' | epsilon`,
   `T -> F T'`, `T' -> * F T' | epsilon`.

2. **Indirect left recursion needs Paull.** `S -> A a | b`,
   `A -> A c | S d | e` becomes `A -> b d A' | e A'`,
   `A' -> c A' | a d A' | epsilon` — the Dragon Book 4.18 answer.

3. **The language is preserved.** Both grammars generate the same strings up to
   length 6 (70 for expr, 20 for indirect); the transformed grammars are not
   left-recursive.

4. **A grammar with no left recursion is returned unchanged.**
