# Module 07 — Decisions

Choices in `GrammarClass.java` / `Pumping.java` and their JS twins, sorted into
**(a) forced**, **(b) external contract**, **(c) our convention**.

---

## Classifying grammars

### Only three kinds: RIGHT_LINEAR, LEFT_LINEAR, CONTEXT_FREE — (c)
The full Chomsky hierarchy has four types, but a grammar file with a single
nonterminal on every left-hand side is already at most Type 2. The only
distinction this module needs is "regular or not," and that turns on the
right-hand-side *shape*:
- **right-linear:** every rhs is `w` or `w B` — terminals, then at most one
  nonterminal, at the very end.
- **left-linear:** every rhs is `w` or `B w` — nonterminal at the very start.
- Anything else with a single-nonterminal lhs is **context-free**.

### The classifier keys on *position* of the nonterminal, not just count — (a)
`S -> a S b` has one nonterminal but it's in the middle → not right-linear, not
left-linear → context-free. `S -> a S` (end) → right-linear. `S -> S a` (start)
→ left-linear. A grammar that mixes `S -> a S` and `S -> S a` is neither, so it
classifies context-free even though both languages are individually regular —
that's correct: the *grammar* isn't linear even if the language is.

### An undefined nonterminal silently becomes a terminal — (c), noted
Same footgun as Module 4. `S -> a T` where `T` has no rule → `T` is a terminal →
`S -> a T` is `w` → right-linear. Module 17 adds the "useless symbol" check.

---

## The pumping lemmas as adversaries

### We refute, we don't "apply" — (a)
The pumping lemma is a *necessary* condition for regularity: regular ⟹ pumpable.
So to prove *not regular* we prove *not pumpable*: for every claimed pumping
length `p`, exhibit a string in `L` for which *no* decomposition survives
pumping. `regularRefutation(p)` does exactly that and reports
`allEscaped = true` when every split failed.

### The chosen string is always `s = a^p b^p` (or `a^p b^p c^p`) — (b)
This is *the* string the standard proof uses, and it's the right one: `|s| ≥ p`,
`s ∈ L`, and the `|xy| ≤ p` constraint forces `y` (or `vwx`) into a single
letter-block, which is what makes every pump break the balance.

### We enumerate *every* valid decomposition, not just the textbook one — (c)
The proof only needs *one* decomposition to fail, but showing that *all*
`p(p+1)/2` splits fail (regular) or all O((3p)⁴) 5-splits fail (context-free) is
stronger and removes any "but what about this other split?" doubt. The tests
assert the full count.

### Escape is tried at k ∈ {2, 0, 3}, in that order — (c)
`k = 2` (pump up) breaks `a^n b^n` for every split; `k = 0` (pump down) also
works; `k = 3` is a fallback. For these two languages k=2 always suffices, so
the reported representative always shows `pump^2`. A different target language
might need a different k, which is why the helper tries a small set rather than
hard-coding 2.

### `firstEscape` takes a "build the pumped string" closure — (c)
Same helper for the 3-part and 5-part cases: pass a lambda `k -> pumped string`.
Keeps the "which k escapes" logic in one place.

### `MAX` / budget: none needed — (a)
Every loop here is bounded by `p` directly (`p(p+1)/2` splits, or `(3p)⁴`
5-splits). No search, no recursion, no safety valve. The demo caps `p` at 6
(regular) and 4 (context-free) purely to keep the output short.

---

## The membership predicates

### `inAnBn` / `inAnBnCn` are hand-written scanners, not regexes — (a)
They're deciding membership in *non-regular* languages, so a regex literally
cannot express them — that's the module's whole point. Two or three `while`
loops counting each letter-run, then checking the counts are equal and nothing
follows.

### `n = 0` is a member — (c)
`""` ∈ `{a^n b^n}` and `""` ∈ `{a^n b^n c^n}` (n = 0). This matches the standard
definition and doesn't affect the pumping argument (which uses `p ≥ 1`).

---

## Decisions We Made

| # | Decision | Bucket | Alternative rejected |
|---|----------|--------|----------------------|
| 1 | Three grammar classes, keyed on rhs shape | (c) | full 4-type hierarchy — file grammars can't reach Type 1 |
| 2 | Classify by nonterminal *position* | (a) | count only — `S -> a S b` would misclassify as regular |
| 3 | Refute the lemma, don't apply it | (a) | there's no other way to prove "not regular" from pumping |
| 4 | Chosen string `a^p b^p` | (b) | any other string — the constraint wouldn't bite |
| 5 | Check *every* decomposition | (c) | check one (enough for the proof) — weaker demonstration |
| 6 | Try k ∈ {2, 0, 3} | (c) | hard-code k=2 — works here, brittle for other languages |
| 7 | Hand-written membership scanners | (a) | a regex — impossible for non-regular languages |

---

## What We Proved

1. **The class of a grammar is mechanical.** `S -> a S | b` → RIGHT_LINEAR
   (regular). `S -> a S b | ε` → CONTEXT_FREE. `E -> E + E | id` → CONTEXT_FREE.
   Decided by looking at where nonterminals sit in each right-hand side.

2. **`{ a^n b^n }` is context-free but not regular.** A CFG generates it
   (`enumerate` shows `ε, ab, aabb, aaabbb, …`). No regular grammar can:
   `regularRefutation(p)` shows that for **every** `p` from 1 to 8, **every**
   one of the `p(p+1)/2` legal `x y z` splits of `a^p b^p` produces a
   non-member when pumped. The pumping lemma has no valid `p`, so the language
   is not regular.

3. **`{ a^n b^n c^n }` is context-sensitive but not context-free.** The 5-part
   context-free pumping lemma fails the same way: for `p` = 1 to 4, all
   O((3p)⁴) decompositions of `a^p b^p c^p` escape under pumping.

4. **The hierarchy is a strict tower.** Type 3 ⊊ Type 2 ⊊ Type 1 ⊊ Type 0, with
   `a*` / `a^n b^n` / `a^n b^n c^n` as the witnesses separating the bottom
   three. This is *why* the course has both a "regular" track (Part II lexing)
   and a "context-free" track (Parts III–IV parsing): they are provably
   different amounts of power, and you reach for the weaker one whenever it
   suffices.
