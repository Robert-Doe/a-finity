# Module 07 — Run It

From `07-chomsky-pumping/`. Verified: OpenJDK 24, Node v22.14.0.
No fixture argument — `Main` runs a fixed script.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
grammar classification:
  fixtures/a-star-b.grammar   S -> a S | b                     RIGHT_LINEAR   (regular)
  fixtures/b-a-star.grammar   S -> S a | b                     LEFT_LINEAR    (regular)
  fixtures/anbn.grammar       S -> a S b | epsilon             CONTEXT_FREE   (not regular)
  fixtures/expr.grammar       E -> E + E | E * E | ( E ) | id  CONTEXT_FREE   (not regular)

a context-free grammar DOES generate { a^n b^n }:
  fixtures/anbn.grammar  enumerated to length 8:  epsilon, ab, aabb, aaabbb, aaaabbbb

no regular grammar can -- the regular pumping lemma has no valid pumping length:
  p=1: s=ab         all 1/1 splits escape;  x="" y="a" z="b", pump^2 -> "aab"  (not in a^n b^n)
  p=2: s=aabb       all 3/3 splits escape;  x="" y="a" z="abb", pump^2 -> "aaabb"  (not in a^n b^n)
  p=3: s=aaabbb     all 6/6 splits escape;  ...
  ...
  for EVERY p, a^p b^p is in L but no split survives pumping  ->  { a^n b^n } is NOT regular

one level up -- the context-free pumping lemma has no valid pumping length for { a^n b^n c^n }:
  p=1: s=abc          all 6 5-splits escape;  ...
  ...
  ->  { a^n b^n c^n } is context-sensitive but NOT context-free

the hierarchy (each type STRICTLY contains the one below):
  Type 3  regular            a*, (a|b)*abb         finite automaton / regex
  Type 2  context-free       a^n b^n, balanced()   pushdown automaton
  Type 1  context-sensitive  a^n b^n c^n           linear-bounded automaton
  Type 0  recursively enum.  { <M,w> : M halts }   Turing machine
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out PumpingTest
node js/pumping.test.mjs
```

Both end `OK   54 passed`.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| a grammar you think is regular classifies as CONTEXT_FREE | it has a production with a nonterminal that isn't at the very end (right-linear) or start (left-linear), or two nonterminals | `S -> a S b` is *not* regular — the `S` is in the middle |
| `allEscaped` is false for some p | the escape-k set `{2, 0, 3}` doesn't cover your language's pump-breaking values | for `a^n b^n` and `a^n b^n c^n`, k=2 always breaks it; a different language may need other k |
| `cflRefutation` is slow for large p | the 5-part decomposition loop is O((3p)^4) | keep p small; p up to 6 is instant, the demo stops at 4 |
| classification wrong for a grammar with an undefined nonterminal | a symbol you meant as a nonterminal has no rule, so it's treated as a terminal | define every nonterminal (Module 17 adds the automatic check) |
