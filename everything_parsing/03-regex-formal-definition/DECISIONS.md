# Module 03 — Decisions

Choices in `Regex.java` / `regex.mjs`, sorted into **(a) forced**,
**(b) external contract**, **(c) our convention**.

---

## The AST

### Exactly six node kinds — (a)
Kleene's definition of a regular expression is: three base cases (`∅`, `ε`, a
single symbol) and three operators (union, concatenation, star). That's the
whole grammar of regular expressions. `Empty`, `Epsilon`, `Char`, `Union`,
`Concat`, `Star` — no more, no fewer.

### `+` and `?` are desugared at parse time, not stored — (c)
`r+` becomes `Concat(r, Star(r))`; `r?` becomes `Union(r, Epsilon)`. They never
reach the AST as their own node kinds, so `toLanguage` and `matches` only ever
handle six cases. The test `parse("a+").tree() == "(cat a (star a))"` pins this.

### `Char` holds one character — no classes, ranges, `.`, anchors, escapes — (c)
`[a-z]`, `.`, `\d` are all unions over Σ (Module 2's Q&A showed this).
`^`/`$` are a matching-mode concern, not a language-structure one.
Backreferences leave regular languages entirely. Module 3 is the *core*; the
sugar is mechanical and the tutorial lists the desugarings.

---

## The parser

### Hand-rolled recursive descent — (c), technique formalised in Module 19
Four mutually-recursive methods, one per precedence level:
`regex → concat → repeat → atom`. This is the third parser you've hand-written
(RPN in M1, grammar files in M4, regex here) before the module that *names* the
technique. That's deliberate — by Module 19 the pattern is muscle memory.

### Precedence: `*` > concatenation > `|` — (b)
Universal across POSIX, PCRE, every textbook, every language's regex library.
`ab*` is `a(b*)`; `a|bc` is `a|(bc)`. Deviating would make every regex the
learner already knows parse wrong. The precedence is encoded purely in which
method calls which — `repeat` (star) is called by `concat`, which is called by
`regex` (union). Grammar shape *is* precedence (a preview of Module 20).

### `|` and concatenation are left-associative — (c)
`a|b|c` parses as `(alt (alt a b) c)`. Semantically it doesn't matter — both
operations are associative — but a fixed choice makes `tree()` output
deterministic and the Java/JS builds identical.

### Empty concatenation is `Epsilon` — (c)
`()` parses to `eps`; the left side of `(|a)` parses to `eps`. Rejecting empty
alternatives would be defensible, but `a?` desugars to `a|ε` which needs an
`Epsilon` node anyway, and `(|a)` = "ε or a" is harmless and occasionally
useful. `concat()` returns `Epsilon` when it sees `)`, `|`, or end immediately.

### s-expression rendering, ASCII, fully parenthesised — (c)
`tree()` prints `(alt a (cat b (star c)))`, not `a|bc*`. The entire job of
`tree()` is to make precedence *visible and unambiguous*; infix rendering would
reintroduce exactly the ambiguity the reader is trying to see resolved. ASCII
(`alt`/`cat`/`star`, not `| · *`) keeps golden files clean cross-platform,
consistent with every other module.

---

## The two semantics

### `toLanguage(maxLen)` — bounded, reuses Module 2 — (c)
Six lines: each node kind delegates to exactly one `Language` operation
(`Char → of`, `Union → union`, `Concat → concat`, `Star → star`). This exists to
*show the thesis*: a regex is those three operations and nothing else. It's
bounded because `star` is.

### `toLanguage` truncates at every operator node — (c)
`Concat` of two length-≤6 languages can produce length-12 strings; truncating
after each `union`/`concat` keeps every intermediate language small. `Star` is
already bounded by construction, so it isn't re-truncated.

### `matches(w)` — exact, unbounded, via leftover suffixes — (c)
`leftovers(node, w)` = the set of suffixes of `w` that remain after `node`
matches a prefix. `w` matches iff `"" ∈ leftovers(node, w)`. This is exact (no
length bound) and finite. It's the honest "does this string match" that a
bounded enumeration can't give you.

### The `rest != w` guard in the `Star` case — (a), essentially
Without it, `(a*)*` on `"aaa"` loops forever: the inner `a*` can match zero
characters, leaving `w` unchanged, so `Star` recurses on the same `w` endlessly.
The guard — "a repetition must consume at least one character" — is the standard
fix and is sound: a zero-width repetition contributes nothing that "zero
repetitions" (the `out.add(w)` line) doesn't already cover. Test:
`parse("(a*)*").matches("aaa")` terminates and is true.

---

## Plumbing

### Fixture separator is ` | ` (space-pipe-space) — (b)
A bare `|` separator would collide with regex alternation. Regexes in this
course never contain spaces, so requiring the separator to be space-delimited
disambiguates with zero ceremony. `Main` uses `indexOf(" | ")`.

### `Language.java` gained `truncate` and was re-synced into Module 2 — (c)
Module 3 needed it; rather than let the two copies drift, the method was added
to Module 2's copy too and both re-verified. The per-module copies stay
byte-identical.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Six node kinds, no more | (a) | store `+`/`?` as nodes — every consumer grows two cases |
| 2 | `+` `?` desugar at parse | (c) | desugar lazily in each consumer — same effect, more places to forget |
| 3 | `Char` = one char, no classes | (c) | build `[a-z]`, `.`, `\d` — all just unions; sugar, not core |
| 4 | Recursive-descent precedence parser | (c) | a table-driven parser now — Module 25's job, overkill here |
| 5 | `* > concat > \|` | (b) | any other order — breaks every regex the learner knows |
| 6 | Left-associative `\|`, concat | (c) | right — identical meaning, non-deterministic tree |
| 7 | Empty alternative → `Epsilon` | (c) | reject `()` / `(\|a)` — need an `Epsilon` node for `a?` regardless |
| 8 | s-expression `tree()`, ASCII | (c) | infix rendering — reintroduces the ambiguity being taught |
| 9 | Two semantics: bounded + exact | (c) | only bounded — can't answer "does w match" honestly; only exact — can't show "= 3 ops" |
| 10 | `rest != w` guard on Star | (a) | omit it — `(a*)*` infinite-loops |

---

## What We Proved

1. **Every regular expression is a tree of six node kinds.** `+` and `?` add
   nothing: `a+` → `(cat a (star a))`, `a?` → `(alt a eps)`. Verified by tests.

2. **Regex precedence is `* > concatenation > |`, and it lives in the grammar
   shape.** `a|bc*` parses to `(alt a (cat b (star c)))` — the same way
   `2+3·4^5` groups. Shown in the demo's precedence table and pinned by tests.

3. **Each node kind is exactly one Module-2 operation.** `toLanguage` is a
   six-case recursion; it reproduces `(a|b)a*` as
   `{ a, b, aa, ba, aaa, baa, aaaa, baaa }` — identical to what Module 2 built by
   hand from `union`/`concat`/`star`.

4. **Exact matching is decidable without a length bound**, even for nasty inputs
   like `(a*)*`, via the leftover-suffix method plus the consume-something
   guard. `(a|b)*abb` accepts `abb`, `aabb`, `babb`, `abababb`; rejects `ab`,
   `abba`, `""`.

5. **You have now hand-written three parsers before Module 19.** The technique
   is not new when it's "taught" — it's named and systematised.
