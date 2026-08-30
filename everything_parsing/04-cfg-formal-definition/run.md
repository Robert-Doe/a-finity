# Module 04 — Run It

From `04-cfg-formal-definition/`. Verified: OpenJDK 24, Node v22.14.0.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main fixtures/anbn.grammar
```

Expected (`expected/anbn.out`):

```
=== Module 04 - Context-Free Grammars ===

grammar: fixtures/anbn.grammar
  nonterminals: S
  terminals   : a b
  start       : S
  productions :
    (0) S -> a S b
    (1) S -> epsilon

leftmost derivation of a a b b  [choices: 0 0 1]
    S
=>  a S b           (0) S -> a S b
=>  a a S b b       (0) S -> a S b
=>  a a b b         (1) S -> epsilon
    4 sentential forms, 3 steps

language up to length 6:
    len 0: epsilon
    len 2: a b
    len 4: a a b b
    len 6: a a a b b b
    4 strings of length <= 6 -- raise the bound and the list always grows (the grammar is infinite)

membership (bounded derivation search):
    a a b b       DERIVABLE      (S =>* a a b b in 3 steps)
    a a b         NOT DERIVABLE  (searched every sentential form with <= 3 terminal symbols)
    b a           NOT DERIVABLE  (searched every sentential form with <= 2 terminal symbols)
    ""            DERIVABLE      (S =>* "" in 1 step)

summary: productions=2  language=infinite  testsDerivable=2/4
```

Try the other fixture too — it has a doubly-recursive rule:

```
java -cp java/out Main fixtures/balanced.grammar
```

---

## JavaScript — identical bytes

```
node js/main.mjs fixtures/anbn.grammar
node js/main.mjs fixtures/balanced.grammar
```

---

## Tests

```
java -cp java/out GrammarTest
node js/grammar.test.mjs
```

Both end `OK   30 passed`.

---

## Write your own grammar

Create `fixtures/mine.grammar`:

```
# every rule is  LHS -> ALT | ALT | ...   symbols are whitespace-separated
# 'epsilon' or an empty alternative is the empty string
# first LHS is the start symbol, unless a '%start X' line says otherwise
E -> E + T | T
T -> ( E ) | n
```

`Main` only scripts derivations/tests for the two shipped fixtures, so with a new
file you will get the grammar summary and empty derivation/membership sections.
The tests show how to call `leftmostDerivation`, `enumerate`, and `derives`
directly.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `rule has no '->'` | a line without `->` that isn't a comment or blank | every non-comment line must be `LHS -> ...` or `%start X` |
| `start symbol 'X' is not a nonterminal` | `%start` names a symbol that never appears on a left-hand side | pick a symbol that has at least one rule |
| enumeration prints nothing | the length bound is smaller than the shortest string in the language | raise `bound` in `Main`, or check the grammar can terminate (every recursive rule needs a non-recursive alternative) |
| `derives` says NOT DERIVABLE for a string you know is in the language | your grammar has a unit/epsilon cycle and the search bound cut it off, **or** the string really isn't derivable | the bound is the target length in terminals; a correct grammar for that string will reach it within that many terminal symbols |
| Java shows `fixtures\anbn.grammar` with a backslash | old build | rebuild — `Main` normalizes the path separator |
