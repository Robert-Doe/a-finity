# Module 06 — Run It

From `06-ambiguity-disambiguation/`. Verified: OpenJDK 24, Node v22.14.0.
No fixture argument — `Main` runs a fixed script over all three grammar files.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Key excerpts of the expected output (`expected/main.out`):

```
--- fixtures/expr-ambiguous.grammar ---
  (0) E -> E + E
  (1) E -> E * E
  (2) E -> ( E )
  (3) E -> id

string "id + id * id" has 2 parse trees:

  TREE A   value(id=2) = 6   grouping: (id + (id * id))
  ...
  TREE B   value(id=2) = 8   grouping: ((id + id) * id)
  ...

smallest ambiguous string (length <= 7): id * id * id

--- fixtures/expr-unambiguous.grammar ---
  ...
string "id + id * id" has 1 parse tree:
  TREE A   value(id=2) = 6   grouping: (id + (id * id))

--- language equivalence (ambiguous vs disambiguated) ---
L(ambiguous)   up to length 7: 60 strings
L(unambiguous) up to length 7: 60 strings
same language? true

--- fixtures/dangling-else.grammar ---
string "if x then if x then a else b" has 2 parse trees:
  TREE A - else binds to the INNER if
  TREE B - else binds to the OUTER if
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out AmbiguityTest
node js/ambiguity.test.mjs
```

Both end `OK   18 passed`.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `allTrees` returns 0 | the target string isn't in the language, or the prefix-prune is wrong | check the string against `Derivation.enumerate` first |
| `allTrees` takes very long / hits `MAX_STATES` | the grammar has a unit cycle (`A -> B`, `B -> A`) or an ε-cycle | Module 6's grammars have neither; a cyclic grammar needs Module 16 / 23 first |
| `smallestAmbiguousString` returns null for a grammar you know is ambiguous | its shortest ambiguous string is longer than `maxLen` | raise the bound |
| dangling-else trees both labelled the same | you changed the "6 children ⇒ else at top" heuristic | production 1 (`if x then S else S`) expands to exactly 6 symbols |
