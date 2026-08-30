# Module 02 — Decisions

Choices in `Language.java` / `language.mjs` and `Main`, sorted into
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## Representing a language

### A `Language` is a finite set of strings — (c), with a stated limit
The mathematical object is "any subset of Σ*," which is usually infinite. We can
only *store* finite ones. Every operation that could produce an infinite result
(`star`, `sigmaStar`) takes a **length bound** and returns the finite slice
below it. This is honest as long as the tutorial says so — and it sets up the
real answer, finite automata (Modules 10–13), which represent infinite regular
languages in finite space.

### Strings are the language's native string type — (a)
Java `String`, JavaScript string. The empty string is `""` in both — a genuine
value, length 0. There is no separate `Epsilon` object; `Language.EPSILON` is
just `of("")`. Introducing an epsilon sentinel would mean every operation has to
special-case it.

### Canonical order: length first, then lexicographic — (c)
`ORDER = (x,y) -> x.length != y.length ? cmp(lengths) : cmp(strings)`. Sorting
purely lexicographically would put `"aa"` before `"b"`, which reads wrong for a
"list the language" view and — more importantly — would make the Java
`TreeSet` order and the JS `.sort()` order disagree on nothing here, but the
*length-first* order is what every "enumerate the language" table in the course
uses, so it's fixed now. `render()` and `list()` both use it.

### `TreeSet` (Java) vs. `Set` + `.sort()` on read (JS) — (b)
Java has an ordered set; JS does not. To keep the two builds byte-identical, JS
stores an unordered `Set` and sorts with `ORDER` every time `list()` or
`render()` is called. Slightly wasteful, completely deterministic.

---

## The operations

### `concat` is the glued cartesian product — (a)
`L1 · L2 = { xy : x ∈ L1, y ∈ L2 }`. That is literally a double loop with string
`+`. Consequence, verified in the tests: `∅ · L = ∅` (no `x` to pair) and
`{ε} · L = L` (`"" + y == y`). The empty language is the annihilator; `{ε}` is
the identity. These aren't design choices — they fall out of the definition.

### `power(0) = {ε}` — (a)
`L⁰` is the empty product, and the identity for concatenation is `{ε}`, so
`L⁰ = {ε}` for every `L`, including `L = ∅`. (`∅⁰ = {ε}`, not `∅` — a common
trip-up.) The loop in `power` starts from `EPSILON` and concatenates `n` times.

### `star` is a breadth-first closure with a seen-set, not `∪ power(k)` — (c)
The textbook definition is `L* = L⁰ ∪ L¹ ∪ L² ∪ …`. Implementing it that way
needs a stopping rule, and the obvious one ("stop when `power(k)` has no string
≤ maxLen") **breaks when `L` contains `ε`**: `power(k)` then always contains
`ε`, so it never looks "done." The closure form — start from `{ε}`, keep
prepending strings of `L`, dedupe with a set, drop anything over `maxLen` —
terminates for every `L`, including `L ∋ ε` and `L = ∅`. Tested both ways.

### `sigmaStar(alphabet, maxLen)` = `star` of the one-character languages — (c)
Σ* is exactly the Kleene star of Σ treated as a language of length-1 strings.
Reusing `star` means one bounded-closure implementation, not two.

### `intersect` / `minus` included though the module doesn't dwell on them — (c)
`Main` uses `intersect` once (to clip `(a|b)a*` to length ≤ 4). They're cheap,
they're standard set operations, and Module 7 (Chomsky hierarchy) will want
`minus` for `complement`. Adding them now costs four lines.

---

## The demo

### No fixture file — (c)
Every other module reads `fixtures/`. This one's subject *is* the algebra, so
`Main` hard-codes the example languages (`{a,b}`, `{c}`, `{a}`) and prints the
result of each operation. `run.md` and the tutorial quote the output directly.

### The `(a|b)a*` example is built by literally composing the methods — (c)
`Language.of("a").union(Language.of("b")).concat(Language.of("a").star(3))`.
The point of the module is that a regular expression is *nothing but* these
three operations, so the demo spells the composition out and then checks
membership against it. The `.intersect(sigmaStar(Σ, 4))` clip is only there to
bound the display.

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | `Language` holds only finite sets, bounded ops | (c) | pretend to hold infinite ones — impossible until Modules 10–13 |
| 2 | `""` is epsilon; no sentinel object | (a) | an `Epsilon` type — special-cased everywhere |
| 3 | Sort length-first, then lexicographic | (c) | pure lexicographic — wrong reading order, and inconsistent with later modules |
| 4 | JS sorts on read; Java uses `TreeSet` | (b) | a JS ordered-set shim — more code, same result |
| 5 | `star` = BFS closure with seen-set | (c) | `∪ power(k)` — infinite-loops when `ε ∈ L` |
| 6 | `power(0) = {ε}` for all `L` | (a) | `∅⁰ = ∅` — mathematically wrong |
| 7 | Ship `intersect` / `minus` now | (c) | add them in Module 7 — trivial either way |
| 8 | No fixture file | (c) | invent an input format for "two languages" — overkill |

---

## What We Proved

1. **Union, concatenation, and power are closed on finite languages.** Every
   result printed by `Main` is a finite, fully-listed set. `L1 ∪ L2` has 3
   strings, `L1 · L2` has 2, `L1³` has 8 — all computed, none approximated.

2. **`{ε} ≠ ∅`.** Sizes 1 and 0. `∅ · L1 = ∅` (annihilator); `{ε} · L1 = L1`
   (identity). Both shown in the output and pinned by tests.

3. **Kleene star is the odd one out.** It is the only operation whose true
   result is infinite, which is why it — and only it — takes a bound. `{a}*`
   truncated at length 4 is `{ε, a, aa, aaa, aaaa}`.

4. **A regular expression is these three operations and nothing more.**
   `(a|b)a*` was reproduced exactly by `union`, `concat`, and `star` over
   one-character languages, and its membership decisions (`b` ✓, `ba` ✓,
   `ab` ✗, `""` ✗) match. Module 3 formalises the notation; the meaning is
   already here.
