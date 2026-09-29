# Module 19 — Decisions

Choices in `Lexer.java` / `RecursiveDescent.java` / `Derivation.java` and their
JS mirrors, in three buckets: **(a) forced**, **(b) external contract**,
**(c) our convention**.

---

## The grammar

### We parse the non-left-recursive form, not the Module 6 form — (a)
Module 6 disambiguated with `E -> E + T` (left recursion gives left
associativity). A recursive-descent method for that rule calls itself as its
first action with the cursor unmoved — unbounded recursion, no progress. Left
recursion and top-down parsing are **fundamentally incompatible**; this is not a
style choice. So Module 19 uses:

```
E  -> T E'
E' -> + T E'  |  epsilon
T  -> F T'
T' -> * F T'  |  epsilon
F  -> ( E )   |  num  |  id
```

The mechanical transform that produces this shape (and why it preserves the
language) is Module 23. We state the dependency in the output and move on.

### Associativity is deliberately "wrong" in the tree — (a), documented
`E' -> + T E'` is right-recursive, so `a + b + c` nests to the right:
`a + (b + c)` in tree shape. That is the price of making the grammar LL-parsable.
Left associativity is recovered later — when lowering the parse tree to an AST
(Module 39), or by writing `E'` as a `while` loop that folds left (shown in the
Main output and §04 of the tutorial). For Module 19 the concrete tree is the
artifact, and it leans right; we say so.

---

## The tokenizer

### A 40-line hand tokenizer, not the Part II lexer — (c)
Modules 9–18 built a spec-driven combined-DFA lexer. Importing it here would
bury the parsing content under lexer setup. This module's `Lexer` skips spaces,
runs digits into `num`, letters into `id`, and treats `+ * ( )` as one-char
tokens. It is the smallest thing that yields a clean, position-tagged token
stream. Real front ends do wire the two together — that integration is its own
concern.

### The stream always ends with an explicit `EOF` token — (c)
So `peek()` never has to bounds-check and `parse()` can assert `expect("EOF")`
to reject trailing garbage (`1 2`). `EOF`'s position is `src.length()`, which is
also the right place to point when the input ends too early (`1 +`).

### Every token carries its start position — (b)
Error messages have to say *where*. The lexer records `pos` at the first
character of each token; the parser reports `peek().pos` when `expect` fails.

---

## The parser

### One method per nonterminal, and it logs its production *before* recursing — (c)
`parseE` does `rules.add("E -> T E'")` as its first line, then calls `parseT`
and `parseEprime`. Because the log entry is written before the sub-calls, the
`rules` list ends up in exactly the order the productions are applied — which,
for recursive descent, is leftmost order. `Derivation.replay` then reconstructs
the sentential forms and checks the endpoint.

### The alternative choice is by token inspection, not a computed table — (c)
`parseEprime` takes `+ T E'` iff the next token is `+`, else `epsilon`. This is
correct here because the FIRST sets of the alternatives are obviously disjoint
(`{+}` vs `{}`) and `+` is not in FOLLOW(E') in a way that collides. Module 20
states the **predictive parsing condition** formally; Module 21 computes FIRST;
Module 25 builds the table. Module 19 does it by eye, on purpose.

### `epsilon` productions are always taken in the `else` branch — (a for LL(1) here)
When the lookahead is not in FIRST of any non-epsilon alternative, the nullable
alternative is the only option. For this grammar that is unambiguous. A grammar
where the epsilon choice depends on FOLLOW (and could conflict) is exactly what
Module 25 detects as non-LL(1).

### `Derivation.replay` refuses a non-leftmost list — (c)
If a production's left-hand side is not the leftmost nonterminal of the current
form, `replay` throws. This turns "the call stack is the leftmost derivation"
into an executable assertion rather than a claim in prose: the test suite feeds
it a deliberately out-of-order list and checks that it rejects it.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Parse the non-left-recursive grammar | (a) | Module 6's `E -> E + T` — infinite recursion |
| 2 | Right-recursive `E'`, tree leans right | (a) | can't have left recursion *and* top-down |
| 3 | Tiny hand tokenizer, not the Part II lexer | (c) | import Module 16 — buries the parsing content |
| 4 | Explicit `EOF` token | (c) | bounds-check every `peek()` |
| 5 | Log the production before recursing | (c) | reconstruct order afterward — fragile |
| 6 | Alternative choice by token inspection | (c) | build FIRST/FOLLOW now — that's Modules 20–21 |
| 7 | `replay` rejects non-leftmost lists | (c) | trust the claim without checking it |

---

## What We Proved

1. **One method per nonterminal parses the language.** `1 + 2 * 3`,
   `(1 + 2) * 3`, `a`, `a + b + c` all parse; the derived sentence equals the
   token kinds every time (`match? true`).

2. **The call stack is the leftmost derivation.** The productions the methods
   fire, in call order, replay as a valid leftmost derivation ending exactly at
   the input — and `replay` throws if handed a list that isn't leftmost.

3. **Precedence and associativity are visible in the tree.** `*` always sits
   under a `T'`, one level below `+`. `a + b + c` nests right (and we show the
   loop form that would nest left).

4. **Errors have positions.** `1 +` → position 3, "missing factor"; `(1 + 2` →
   position 6, "expected ')'"; `1 2` → position 2, "expected EOF".

5. **Left recursion is fatal to top-down parsing** — stated, with the exact
   failure (parseE calls parseE, cursor unmoved), and forwarded to Module 23.
