# Module 05 — Run It

From `05-derivations-parse-trees/`. Verified: OpenJDK 24, Node v22.14.0.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main fixtures/expr.grammar
```

Expected (`expected/expr.out`):

```
=== Module 05 - Derivations, Parse Trees, Leftmost/Rightmost ===

grammar: fixtures/expr.grammar
  (0) E -> E + T
  (1) E -> T
  (2) T -> T * F
  (3) T -> F
  (4) F -> ( E )
  (5) F -> id

target: id + id * id

parse tree:
E
+- E
|  +- T
|     +- F
|        +- id
+- +
+- T
   +- T
   |  +- F
   |     +- id
   +- *
   +- F
      +- id

yield (terminal frontier): id + id * id
  matches target? true

leftmost derivation  (always expand the leftmost nonterminal):
    E
=>  E + T                   (E -> E + T)
=>  T + T                   (E -> T)
=>  F + T                   (T -> F)
=>  id + T                  (F -> id)
=>  id + T * F              (T -> T * F)
=>  id + F * F              (T -> F)
=>  id + id * F             (F -> id)
=>  id + id * id            (F -> id)
    8 steps

rightmost derivation  (always expand the rightmost nonterminal):
    E
=>  E + T                   (E -> E + T)
=>  E + T * F               (T -> T * F)
=>  E + T * id              (F -> id)
=>  E + F * id              (T -> F)
=>  E + id * id             (F -> id)
=>  T + id * id             (E -> T)
=>  F + id * id             (T -> F)
=>  id + id * id            (F -> id)
    8 steps

productions used (multiset, sorted by index):
  leftmost : [E->E+T, E->T, T->T*F, T->F, T->F, F->id, F->id, F->id]
  rightmost: [E->E+T, E->T, T->T*F, T->F, T->F, F->id, F->id, F->id]
  same multiset? true  (they are derived from the SAME tree)

summary: parseTrees=1  canonicalDerivations=2  steps=8  productionsEach=8  sameMultiset=true
```

The two derivations differ from step 2 onward, use the same 8 productions, and
end at the same tree. Also try `fixtures/anbn.grammar` — there, because every
sentential form has at most one nonterminal, leftmost and rightmost are
*identical*.

---

## JavaScript — identical bytes

```
node js/main.mjs fixtures/expr.grammar
node js/main.mjs fixtures/anbn.grammar
```

---

## Tests

```
java -cp java/out ParseTreeTest
node js/parsetree.test.mjs
```

Java: `OK   20 passed`. JS: `OK   17 passed` (same core claims; Java adds a few
extra edge-case checks).

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `production N (...) does not match frontier nonterminal 'X'` | the hard-coded choices don't fit the grammar | choices are per-fixture in `Main`; a new grammar needs its own leftmost-derivation choice list |
| `choices ran out before the tree was complete` | too few production indices | every nonterminal introduced must be expanded; count them |
| leftmost and rightmost derivations look identical | the grammar (like `anbn`) never has two nonterminals in one sentential form | that's correct — the orders only diverge when there's a choice of which nonterminal to expand |
| tree renders with weird indentation | you changed the `"|  "` / `"   "` continuation strings | last child uses 3 spaces, others use `\|` + 2 spaces |
