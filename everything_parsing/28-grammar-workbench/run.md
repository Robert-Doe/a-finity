# Module 28 — Run It

From `28-grammar-workbench/`. Verified: OpenJDK 24, Node v22.14.0.
Capstone B: diagnose Ajoda's grammar, repair it, and parse real programs.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
--- 1. the grammar as the spec writes it ---
grammar: fixtures/ajoda.ebnf
  24 EBNF rules -> 82 BNF productions | 24 rules + 21 helpers = 45 nonterminals | 37 terminals (3 token classes)
  ...
  LL(1): NO, 1 conflict
    Statement on IDENT:
      [Statement -> AssignStmt]  via Statement -> AssignStmt -> IDENT
      [Statement -> ExprStmt]  via Statement -> ExprStmt -> Expression -> LogicalOr -> ... -> Primary -> IDENT

--- 2. one repair: fold assignment into ExprStmt ---
  LL(1): YES  table 45 x 38, 267 of 1710 cells used

parse fixtures/factorial.tokens (42 tokens from Module 16):
  ACCEPTED: 176 expansions, 41 matches, stack depth up to 16

parse fixtures/missing-semi.tokens (23 tokens from Module 16):
  REJECTED at 3:5: in Call, expected one of "(" ")" "," "=" ";" ... but found "return"

--- 3. a left-recursive grammar ---
  left recursion: YES
  removed with Paull's algorithm (Module 23):
    Expr -> Term Expr'
    ...
  LL(1): YES  table 5 x 7, 15 of 35 cells used
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out WorkbenchTest    # OK 18 passed
node js/workbench.test.mjs         # OK 18 passed
```

---

## EBNF format

```
# a comment
Rule    ::= Other "literal" TOKEN_CLASS
Choice  ::= A | B
          | C                     # a continuation line starts with |
Repeat  ::= { X }                 # zero or more
Maybe   ::= [ X ]                 # optional
Group   ::= ( "+" | "-" ) Term    # grouping
```

The first rule is the start symbol. `UPPER_CASE` names that no rule defines
are token classes (`IDENT`, `INT_LIT`, `FLOAT_LIT`).

## Token file format (written by Module 16)

```
# comment
2:1 FN fn
2:4 IDENT fact
10:1 EOF
```

`IDENT`, `INT_LIT` and `FLOAT_LIT` match the grammar by kind; every other
token matches the quoted literal equal to its lexeme.

To regenerate a token file after editing a `.ajoda` program, scan it with
Module 16's `Scanner` and print `line:col KIND lexeme` for each token.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `'Foo' is not defined` | a misspelled rule, or a token class not in `UPPER_CASE` | fix the name |
| `expected 'Name ::='` | a line that is neither a new rule nor a continuation | start continuations with `\|` |
| every keyword is a parse error | the token file's lexemes don't match the grammar's literals | regenerate the tokens with Module 16 |
| a conflict names `X_rep1` | the clash is inside a `{ }` of rule `X` | look at the `helpers:` list for the exact bracket |
| `left recursion: YES` on an EBNF grammar | a rule starts with itself (`E ::= E "+" T`) | rewrite as `E ::= T { "+" T }`, or let Paull handle it |
