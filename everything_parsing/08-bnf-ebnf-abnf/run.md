# Module 08 — Run It

From `08-bnf-ebnf-abnf/`. Verified: OpenJDK 24, Node v22.14.0.
No fixture argument — `Main` runs a fixed script over both `.ebnf` fixtures.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
--- fixtures/expr.ebnf ---
EBNF, ISO-style  ( { } = zero or more,  [ ] = optional ):
  expr     = term { ( '+' | '-' ) term }
  term     = factor { ( '*' | '/' ) factor }
  factor   = '(' expr ')' | NUM

desugared to pure BNF:
  expr -> term rep_1
  term -> factor rep_4
  factor -> ( expr ) | NUM
  rep_1 -> grp_2 rep_1 | epsilon
  grp_2 -> grp_3 term
  grp_3 -> + | -
  rep_4 -> grp_5 rep_4 | epsilon
  grp_5 -> grp_6 factor
  grp_6 -> * | /

equivalence check (BNF enumerated to length 7):
  220 strings in L(BNF); every one also accepted by the EBNF matcher: true
  sample accepts all pass: true   sample rejects all pass: true
```

Followed by `signed.ebnf` (showing `?` and `+`) and a notation cheat-sheet
comparing BNF / EBNF / ISO-EBNF / ABNF.

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out EbnfTest
node js/ebnf.test.mjs
```

Both end `OK   26 passed`.

---

## EBNF fixture syntax

```
rule = alt ;                 // every rule ends with ;
alt  = seq ('|' seq)*        // | for alternation
seq  = item item ...         // juxtaposition for sequence
item = atom ('?' | '*' | '+')?   // postfix quantifiers
atom = '(' alt ')' | 'terminal' | NonterminalName
```

`// ...` line comments. Quoted (`'x'` or `"x"`) = terminal. An unquoted name
with no rule of its own is a terminal too (like `NUM`).

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `EBNF: expected ';'` | a rule wasn't terminated | every rule ends with `;` |
| `EBNF: expected an identifier` | a quantifier or `\|` where an atom was expected | `*x` is wrong; quantifiers are postfix |
| the BNF has more rules than you expected | each `?` `*` `+` and each parenthesised group becomes one fresh nonterminal | that's the desugaring — count the operators |
| `every one also accepted by the EBNF matcher: false` | the desugaring and the matcher disagree — a real bug | check that `Plus` desugars to `x P \| x` (one or more), not `x P \| epsilon` |
| a `shouldAccept` sample fails `inLang` | the sample is longer than the enumeration bound | raise `maxLen` in `Main` for that fixture |
