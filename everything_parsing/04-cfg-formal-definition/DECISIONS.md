# Module 04 — Decisions

Choices in `Grammar.java` / `Derivation.java` and their JS twins, sorted into
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## Representing the grammar

### A grammar is stored, not coded — (c)
Module 1's checker *was* a grammar, welded into control flow. Here the grammar
is a `List<Production>` you load from a file, because every later module
(FIRST/FOLLOW, LL tables, LR items, the parser generator) is an *algorithm that
takes a grammar as input*. If the grammar is code, none of those can be written.

### One `Production` per alternative — (c)
`S -> a S b | epsilon` becomes **two** `Production` records, indices 0 and 1,
not one record with two right-hand sides. Every parsing algorithm downstream
selects "a production"; making the alternative the unit of selection now means
`productions.get(k)` is the whole choice. The bar `|` is pure file-format
sugar, expanded at load time.

### Epsilon is the empty right-hand side — (c)
`S -> epsilon` and `S ->` (nothing after the arrow) both produce
`rhs = []`. There is no `epsilon` symbol in the grammar's alphabet — it would be
a phantom terminal that no algorithm should ever see. `isEpsilon()` is just
`rhs.isEmpty()`. Mixing `epsilon` with real symbols (`S -> a epsilon`) is a
load-time error, because it can only be a mistake.

### Nonterminals = "appears as a left-hand side"; terminals = "everything else in a right-hand side" — (c)
The grammar file never declares which symbols are which. This is the universal
convention in grammar tooling (yacc, ANTLR, BNF): if a symbol has a rule, it's a
nonterminal; if it only ever appears in bodies, it's a terminal. Consequence: a
nonterminal you reference but forget to define silently becomes a terminal.
That is a real footgun, and Module 17 (FIRST) adds the check that
catches it ("useless / undefined symbols").

### Nonterminal order is insertion order (`LinkedHashSet`) — (c)
So `Main`'s "nonterminals: ..." line and every later table is deterministic and
matches the file. A `HashSet` would make output order depend on hash codes and
break byte-parity between runs, never mind between languages.

### Start symbol: first lhs, or `%start` — (c)
Matching yacc's `%start`. Defaulting to the first rule's lhs is the common
convenience; `%start` exists for grammars where the natural entry point isn't
written first.

---

## The derivation engine

### Sentential form = `List<String>` — (a), essentially
A step of derivation replaces one symbol in a sequence of symbols with zero or
more symbols. That *is* a list with a splice operation. `substitute(form, i,
rhs)` is `form[0..i) ++ rhs ++ form(i..]`.

### `leftmostDerivation` is driven by explicit production indices — (c)
The caller passes `[0, 0, 1]` and the engine applies production 0, then 0, then
1, each to the **leftmost** nonterminal. This makes the demo a *fixed, readable
script* rather than a search, so the tutorial can show one specific derivation
and the reader can check every step by hand. Choosing a production whose lhs
isn't the current leftmost nonterminal is a thrown error — that's the contract
of "leftmost derivation."

### Leftmost specifically — (c) here, revisited in Module 5
Every terminal string derivable at all has a leftmost derivation, so nothing is
lost by fixing the order. *Which* order (leftmost vs. rightmost) is the seed of
the top-down / bottom-up split, and that gets its own module. Module 4 just
needs *a* canonical order so `enumerate` doesn't produce the same string by
fifty reorderings of the same choices.

### `enumerate` and `derives` are breadth-first with a terminal-count bound — (b), (a)
- **Breadth-first** (a queue, not recursion): guarantees the *shortest*
  derivation is found first, so `derives` can report a meaningful step count and
  `enumerate` produces strings in non-decreasing length.
- **The bound**: discard any sentential form with more terminal symbols than the
  target length (for `derives`) or `maxLen` (for `enumerate`). Sound because
  **terminals are never rewritten** — once a form has 5 terminals it will always
  have at least 5. This is what makes the search finite.
- **`seen` / `depth` visited-set**: a form like `A -> A` would otherwise loop
  forever within the bound. Keying on the joined symbol string dedupes.
- **`MAX_FORMS = 200_000` safety valve** — (c): a grammar with an epsilon-cycle
  (`A -> B`, `B -> A`, `A -> epsilon`) can generate unboundedly many distinct
  sentential forms *at the same terminal count* before the visited-set catches
  them all. The cap makes the demo always terminate; a grammar that hits it is
  telling you it needs the transformations of Modules 23 and X2.

### `key` uses `""` as the symbol separator — (b)
Symbols are whitespace-delimited in the file, so they can't contain whitespace,
but they *can* be multi-character (`S`, `Expr`). Joining with `""` would make
`["a","b"]` and `["ab"]` collide. `` cannot appear in a symbol.

### `enumerate` sorts by terminal-symbol count, then lexicographically — (c)
Not by rendered-string *length* — that was the first bug (`"epsilon"` is 7
characters and sorted in among the 4-symbol strings). The count of terminal
symbols is the real notion of "how long is this string," so that's the sort key.

### Two renderings of the empty string — (c)
The language listing prints it as `epsilon` (it's a member of the set, shown
alongside `a b`, `a a b b`). Membership tests and derivations print it as `""`
(it's the input you typed, shown alongside `a a b b`). Same value, two audiences.

---

## Plumbing (identical reasoning to Module 1)

### Output buffered, printed once with `\n`; path separators normalized to `/` — (a)
Java `println` emits `\r\n` on Windows and `Path.toString()` uses `\`. Both are
normalized so the Java and JS builds emit identical bytes for every fixture.

### Per-fixture derivation/test scripts live in `Main`, keyed by filename — (c)
`Main` isn't a general grammar REPL — it's a *demo* of two specific grammars.
Hard-coding the derivation `[0,0,1]` and the four test strings per file keeps the
output deterministic and the tutorial able to quote it. A new grammar file gets
the summary sections and empty derivation sections, and `run.md` points you at
the library functions.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Grammar loaded from a file as data | (c) | grammar-as-code — makes every later algorithm impossible to write |
| 2 | One `Production` per alternative | (c) | rhs-list-per-rule — pushes the `|` split into every consumer |
| 3 | Epsilon = empty rhs, no epsilon symbol | (c) | a real `epsilon` terminal — a phantom every algorithm must special-case |
| 4 | Nonterminal ⇔ has a rule | (c) | explicit `%token` declarations — more ceremony, and Module 17 adds the safety check anyway |
| 5 | `%start` or first lhs | (c) | require `%start` always |
| 6 | Leftmost derivation, index-driven | (c) | a search — unreadable demo; rightmost — deferred to Module 5 |
| 7 | BFS + terminal-count bound + visited set | (a)/(b) | DFS — no shortest-derivation guarantee, easy infinite recursion |
| 8 | `MAX_FORMS` cap | (c) | trust the grammar — hangs on epsilon-cycles |
| 9 | Sort by terminal-symbol count | (c) | sort by string length — the bug that shipped first |
| 10 | `epsilon` vs `""` rendering by context | (c) | one rendering — loses either "it's in the set" or "it's your input" |

---

## What We Proved

1. **A finite grammar generates an infinite language.**
   `S -> a S b | epsilon` has 2 productions. `enumerate(g, 6)` returns 4
   strings; `enumerate(g, 8)` returns exactly 5; every increase of the bound by
   one pair adds exactly one string, forever. Verified by
   `enumerate(g, 8).size() == lang6.size() + 1`.

2. **"Well-formed" now has a precise definition.** In Module 1 it was "my
   counter accepts it." Here it is `S ⇒* w` — *there exists a derivation* — and
   `derives` decides it by exhibiting one (`a a b b` in 3 steps) or proving none
   exists within the sound bound (`a a b`, all forms with ≤ 3 terminals
   searched).

3. **The grammar is data an algorithm consumes.** `Grammar.parse` +
   `productionsFor` + `substitute` are the substrate every remaining Track-1
   module builds on. `Derivation` never mentions `a`, `b`, `S`, or `anbn` — it
   works on any loaded `Grammar`.

4. **Generation is a terrible recognizer.** `derives` works, but it explores an
   exponential space of sentential forms to answer a yes/no question that a real
   parser answers in linear time. That gap is the motivation for every parsing
   algorithm from Module 19 on.
