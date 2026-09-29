# Module 28 — Decisions

Choices in `Ebnf.java` / `ebnf.mjs` and `Workbench.java` / `workbench.mjs`,
sorted into **(a) forced** by the algorithms, **(b) fixed by the language
being analysed** (Ajoda), and **(c) our own convention**.

---

## Reading the grammar

### Input is EBNF, as a language spec writes it — (b)
Ajoda's spec states its grammar in EBNF with `{ }`, `[ ]` and grouped
alternatives. A tool that demands hand-converted BNF first would be checking
the conversion, not the language.

### A rule starts at `Name ::=`, not at a line break — (c)
Long rules wrap. Treating the next `Name ::=` as the boundary lets
alternatives continue on a line starting with `|`, the way specs are laid out.

### UPPER_CASE undefined names are token classes; other undefined names are errors — (c)
`IDENT` and `INT_LIT` come from the scanner. `Expresion` is a typo. Without
this rule the typo would silently become a terminal and show up later as a
baffling conflict.

---

## Desugaring

### Each bracket becomes a helper named `Rule_kindN` — (c)
`Term_rep1`, `Call_opt2`. Every generated nonterminal says which rule and
which bracket it came from, so conflicts and parse errors stay readable. The
counter is per rule and shared across bracket kinds.

### `{ X }` becomes right recursion — (a)
`R → X R | ε` keeps the grammar free of left recursion; `R → R X | ε` would
need a Paull pass for every repetition.

### A one-branch group is inlined — (c)
`( "a" "b" )` adds nothing but parentheses, so no helper is created.

---

## Diagnosis

### Conflicts are grouped by (nonterminal, lookahead), listing every production — (c)
Pairwise reporting repeats the same clash for three-way conflicts. One entry
per lookahead shows the whole picture.

### Each production in a conflict gets a derivation chain — (c)
"Statement on IDENT" says *that* there's a problem. The chain
`ExprStmt -> Expression -> ... -> Primary -> IDENT` says *why*, which is what
a language designer needs to fix it.

### Left recursion is removed automatically; conflicts are not — (c)
Removing left recursion never changes the language and has one standard
answer (Paull). Resolving a conflict is a design choice (fold into
expressions? more lookahead? new syntax?), so the tool reports and stops.

### The repair is folding assignment into `ExprStmt` — (b)
`ExprStmt ::= Expression [ "=" Expression ] ";"`. It keeps Ajoda's surface
syntax unchanged and moves "is the left side assignable?" to semantic analysis.

---

## Running the table

### The token file is the only link to Module 16 — (c)
`line:col KIND lexeme` text lines. Either capstone can change internally as
long as that format holds.

### `IDENT`, `INT_LIT`, `FLOAT_LIT` match by kind; everything else by lexeme — (b)
Ajoda's grammar names those three classes and quotes every keyword and
operator, so the mapping follows the grammar's own notation.

### Errors cite `line:col` and the owning rule, not the helper — (c)
"in Call, expected one of ..." rather than "in Call_opt1". The helper name is
an implementation detail.

---

## Scorecard

| # | Decision | Kind | Rejected alternative |
|---|----------|------|----------------------|
| 1 | EBNF input | (b) | hand-converted BNF |
| 2 | rules start at `Name ::=` | (c) | one rule per line |
| 3 | undefined non-UPPER names are errors | (c) | silently treat them as terminals |
| 4 | helpers named `Rule_kindN` | (c) | anonymous `N17`, `N18`, ... |
| 5 | `{ }` → right recursion | (a) | left recursion, then Paull |
| 6 | inline one-branch groups | (c) | a helper for every `( )` |
| 7 | conflicts grouped by lookahead | (c) | one line per pair |
| 8 | derivation chain per production | (c) | just the two production texts |
| 9 | auto-remove left recursion, never auto-resolve conflicts | (c) | guess a fix silently |
| 10 | fold assignment into `ExprStmt` | (b) | LL(2) just for statements |
| 11 | token file as the interface | (c) | import the scanner directly |
| 12 | errors name the owning rule | (c) | name the helper |
