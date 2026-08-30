# Everything Parsing — ROADMAP

> **Status: PHASE 0 COMPLETE — scope locked 2026-08-30.**
> Committed course = **Parts I–VI + Part VI-b, Modules 1–48.** Parts VII–VIII
> are demoted to an optional **Appendix** (not committed; built only on later
> request). Build order: Module 1 forward, one module fully finished before the
> next, **no stop at Part boundaries**. Theme: attached light template.

A code-first, build-it-from-scratch course on parsing, **modeled on the
structure and rigor of ASU CSE 340 (Principles of Programming Languages,
Bazzi)** and pushed to expert depth. Every module ships a **working Java
implementation and a working JavaScript implementation, side by side**, built
one small verifiable piece at a time.

This folder sits next to a C compiler course (`../module_01 … module_16`). That
course *uses* a parser and hand-waves it. **This course is the parser**, taught
the way CSE 340 teaches it: formal definitions first, then the algorithm, then a
graded-style project with exact expected output.

---

## Reference basis

| Source | Used for |
|--------|----------|
| ASU **CSE 340** lecture arc (Bazzi) | Overall sequence, the 4 project milestones, the exact-output-matching test discipline, restricted-RE syntax for the lexer project, the FIRST/FOLLOW/nullable formulation, Paull's algorithm ordering for left-recursion removal, the "poly"-style ordered semantic-error project, the type-inference project |
| **Dragon Book** (Aho, Lam, Sethi, Ullman, 2e) | Input buffering, Thompson construction, subset construction, LR item sets, SLR/LALR table construction, SDT |
| Sipser, *Intro to the Theory of Computation* | Myhill–Nerode, pumping lemmas, the hierarchy separations |
| Grune & Jacobs, *Parsing Techniques* 2e | The expert-extension phase: Earley, GLR/GLL, error recovery, non-canonical methods |
| Ford (PEG/packrat), Pratt (1973), Warth (packrat left recursion) | The PEG / combinator / Pratt phase |

Where a lecture claim and real output disagree, the real output wins and the
tutorial says so.

---

## How this maps to CSE 340

CSE 340 is a full PL-principles course. This folder is **everything *parsing***,
so the parsing and static-semantics spine is rebuilt in full depth, and the
pure-runtime topics are treated as out of scope (the neighboring compiler course
covers code generation and runtime). See *Scope* below.

| CSE 340 milestone | Rebuilt here as | Modules |
|-------------------|-----------------|---------|
| Project 1 — Lexical analyzer (RE list → longest-match scanner) | **Module 16** | 9–18 lead in |
| Project 2 — Grammar analysis (terminals, nullable, FIRST, FOLLOW, left factoring, left-recursion removal, LL(1) verdict) | **Module 28** | 19–29 lead in |
| Project 3 — Parser + ordered semantic-error reporting + execution ("poly"-style) | **Modules 40–41** | 37–41 lead in |
| Project 4 — Type inference from usage / type checking | **Module 44** | 42–44 lead in |
| Bottom-up parsing lectures (LR/SLR/LALR, yacc) | **Modules 30–36** | — |
| Runtime environments, parameter passing, GC | *Out of scope* — see neighboring compiler course | — |

---

## The Course (single linear track, Modules 1–48 committed)

### Part I — Foundations: Syntax, Semantics, Formal Languages  ·  ✅ COMPLETE

> All 8 modules built and verified (2026-08-30). 233 Java + 230 JS assertions
> pass; every module's Java and JS builds produce byte-identical output matching
> a committed golden file. Prerequisites layer: 7 of 9 pages written
> (P1–P7). Modules 1–8 in dependency order; nav links wired 01↔…↔09.

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 1 | Syntax vs. Semantics vs. Pragmatics | Proves a string can be syntactically valid and still semantically meaningless, so the two checkers must be separate passes. | `01-syntax-vs-semantics/` | **DONE** — Java+JS verified, 28 tests each, tutorial + DECISIONS + glossary |
| 2 | Alphabets, Strings, Languages, Operations | Proves union, concatenation, and Kleene star are closed operations on languages and computable for finite cases. | `02-strings-languages-operations/` | **DONE** — Java+JS verified, 23 tests each, tutorial + DECISIONS + glossary |
| 3 | Regular Expressions: Formal Definition | Proves every RE denotes a language built from 3 base cases and 3 operators, by parsing an RE into its own AST. | `03-regex-formal-definition/` | **DONE** — Java+JS verified, 34 tests each, 5 fixtures, tutorial + DECISIONS + glossary |
| 4 | Context-Free Grammars: Formal Definition | Proves a 4-tuple (N, Σ, P, S) plus the derivation relation ⇒ generates exactly the context-free languages. | `04-cfg-formal-definition/` | **DONE** — Java+JS verified, 30 tests each, 2 fixtures, tutorial + DECISIONS + glossary (built ahead of 2–3; nav wired) |
| 5 | Derivations, Parse Trees, Leftmost/Rightmost | Proves a parse tree abstracts away expansion order: one tree corresponds to exactly one leftmost and one rightmost derivation. | `05-derivations-parse-trees/` | **DONE** — Java+JS verified, 20/17 tests, 2 fixtures, tutorial + DECISIONS + glossary |
| 6 | Ambiguity & Disambiguation | Proves some grammars admit two parse trees for one string, and that precedence/associativity rewrites remove it without changing the language. | `06-ambiguity-disambiguation/` | **DONE** — Java+JS verified, 18 tests each, 3 fixtures, tutorial + DECISIONS + glossary |
| 7 | The Chomsky Hierarchy & Pumping Lemmas | Proves regular ⊊ context-free by pumping `aⁿbⁿ` out of any candidate regular grammar. | `07-chomsky-pumping/` | **DONE** — Java+JS verified, 54 tests each, 4 fixtures, tutorial + DECISIONS + glossary |
| 8 | BNF, EBNF, ABNF & Syntax Diagrams | Proves these notations are inter-convertible, by building a normalizer that desugars EBNF to pure BNF. | `08-bnf-ebnf-abnf/` | **DONE** — Java+JS verified, 26 tests each, 2 fixtures, tutorial + DECISIONS + glossary |

### Part II — Lexical Analysis  · *→ CSE 340 Project 1*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 9 | Input Buffering: Two-Buffer Scheme & Sentinels | Proves a scanner can retract arbitrary lookahead at O(1) amortized cost using paired buffers and sentinel bytes. | `09-input-buffering/` | **DONE** — Java+JS verified, 22 tests each, tutorial + DECISIONS + glossary |
| 10 | DFA: Deterministic Finite Automata | Proves a DFA is a total transition function δ: Q×Σ→Q, and its accepted set is exactly a regular language. | `10-dfa/` | **DONE** — Java+JS verified, 61/60 tests, 3 fixtures, cross-checked vs Module 3 regex, tutorial + DECISIONS + glossary |
| 11 | NFA and ε-NFA | Proves nondeterminism and ε-moves add convenience but no power, via ε-closure. | `11-nfa-epsilon/` | **DONE** — Java+JS verified, 112 tests each, 2 fixtures, ε-elimination + cross-check vs Module 10/3, tutorial + DECISIONS + glossary |
| 12 | Thompson's Construction: RE → ε-NFA | Proves every RE compiles structurally to an ε-NFA with at most 2·\|RE\| states. | `12-thompson-construction/` | **DONE** — Java+JS verified, 97 tests each, ≤2·\|regex\| bound checked on 10 regexes, 3-way cross-check, tutorial + DECISIONS + glossary |
| 13 | Subset Construction: NFA → DFA | Proves the powerset DFA accepts the identical language, and exhibits an RE whose DFA is exponentially larger. | `13-subset-construction/` | **DONE** — Java+JS verified, 88 tests each, 2^k blow-up shown for k=1..5, full regex→DFA pipeline, tutorial + DECISIONS + glossary |
| 14 | DFA Minimization | Proves a unique minimal DFA exists (Myhill–Nerode) and builds it by partition refinement and by table-filling. | `14-dfa-minimization/` | **DONE** — Java+JS verified, 31 tests each, both algorithms agree, regex-equivalence decision procedure, tutorial + DECISIONS + glossary |
| 15 | Maximal Munch & Token Priority | Proves the two lexer disambiguation rules: longest match wins, ties broken by declared rule order. | `15-maximal-munch-priority/` | **DONE** — Java+JS verified, 17 tests each, longest-match + priority demonstrated, DFA + last-accept mechanism, tutorial + DECISIONS + glossary |
| 16 | The Lexical Analyzer  *(Project 1)* | Proves a full lexer built from a token-spec list: RE→NFA per token, combined automaton, longest-match scan, and the "epsilon is not a token" rejection. | `16-lexer-project1/` | **DONE** — CSE 340 Project 1: Java+JS verified, 15 tests each, spec-driven, combined tagged DFA (43 states), epsilon-token rejected, tutorial + DECISIONS + glossary |
| 17 | Lexical Errors & Recovery | Proves the lexer can report an exact position and resynchronize by skipping to a safe character class. | `17-lexical-errors/` | **DONE** — Java+JS verified, 18 tests each, PANIC_ONE + PANIC_TO_SYNC, one scan finds all errors, tutorial + DECISIONS + glossary |
| 18 | Practical Lexemes: Numbers, Strings, Comments, Keywords | Proves the reserved-word trick (scan as identifier, then look up) and correct handling of nested and line comments. | `18-practical-lexemes/` | **DONE** — Java+JS verified, 30 tests each, 1 fixture set, hand-coded scanner, nested `/* */` via depth counter (not regular), string escapes, INT/FLOAT/exp/HEX state machine, tutorial + DECISIONS + glossary |

### Part II — COMPLETE ✅

> Modules 9–18 done. A full lexer: two-buffer input (9), the automata layer
> (10–14), maximal munch + priority (15), the spec-driven combined-DFA analyzer
> = CSE 340 Project 1 (16), error recovery (17), and the hand-coded practical
> lexemes (18). Every module: Java + JS byte-identical to a committed golden,
> both test suites green.

### Part III — Top-Down Parsing  · *→ CSE 340 Project 2*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 19 | Recursive-Descent Parsing | Proves each nonterminal becomes one procedure and the call stack *is* the leftmost derivation. | `19-recursive-descent/` | **DONE** — Java+JS verified, 21 tests each, 7 fixtures, one method per nonterminal, `Derivation.replay` reconstructs (and validates) the leftmost derivation, concrete tree + precedence/associativity shown, left-recursion trap documented, tutorial + DECISIONS + glossary |
| 20 | The Predictive Parsing Condition | Proves backtrack-free recursive descent requires pairwise-disjoint FIRST sets on a rule's alternatives. | `20-predictive-condition/` | **DONE** — Java+JS verified, 22 tests each, 4 grammars, FIRST/FOLLOW/PREDICT by fixed point, LL(1) verdict with concrete conflict witnesses (shared prefix, dangling else, left recursion), tutorial + DECISIONS + glossary |
| 21 | Nullable & FIRST Sets | Proves nullable and FIRST are least fixed points, computed by iterating productions to convergence. | `21-nullable-first/` | **DONE** — Java+JS verified, 23 tests each, 3 grammars, round-by-round convergence trace, termination + leastness argued, direct/indirect nullability + nullable prefixes, cross-checked vs Module 20, tutorial + DECISIONS + glossary |
| 22 | FOLLOW Sets | Proves FOLLOW is a fixed point over FIRST and the productions, and why the `$` end-marker is needed. | `22-follow-sets/` | **DONE** — Java+JS verified, 18 tests each, 3 grammars, 3-rule fixed point on top of Module 21 FIRST, mutually-recursive FOLLOW (dangling else) resolved, `$` necessity demonstrated, cross-checked vs Module 20, tutorial + DECISIONS + glossary |
| 23 | Left Recursion Elimination | Proves Paull's algorithm removes direct and indirect left recursion under a fixed nonterminal ordering while preserving the language. | `23-left-recursion-elimination/` | **DONE** — Java+JS verified, 11 tests each, 2 fixtures, direct rewrite + Paull's for indirect, natural expr grammar → the Module 19 grammar, indirect = Dragon Book 4.18, language preservation checked by bounded enumeration, tutorial + DECISIONS + glossary |
| 24 | Left Factoring | Proves factoring the longest common prefix makes alternatives FIRST-disjoint wherever it is possible at all. | `24-left-factoring/` | **DONE** — Java+JS verified, 12 tests each, 3 fixtures, longest-prefix extraction to fixed point, nested prefixes + empty suffixes, unfactored dangling-else → the Module 20/22 grammar, "necessary not sufficient" shown, language preserved (bounded enum), tutorial + DECISIONS + glossary |
| 25 | The LL(1) Parsing Table | Proves table[A,a] is single-valued iff the grammar is LL(1); every conflict is a concrete non-LL(1) witness. | `25-ll1-table/` | **DONE** — Java+JS verified, 19 tests each, 3 grammars, M[A][t] from PREDICT sets, grid render, conflict cells named (dangling else), FOLLOW-driven cells, stack-driven parse trace to ACCEPT, tutorial + DECISIONS + glossary |
| 26 | Table-Driven Predictive Parsing | Proves an explicit stack plus the table reproduces recursive descent with no recursion. | `26-table-driven-parsing/` | **DONE** — Java+JS verified, 15 tests each, 6 inputs, explicit (symbol,node) stack + LL(1) table + loop, parse tree built in-pass, production sequence replayed as leftmost derivation = input, errors located, tutorial + DECISIONS + glossary |
| 27 | LL(1) Error Recovery: Panic Mode & Phrase-Level | Proves FOLLOW-based synchronizing sets let the table parser continue past an error. | `27-ll1-error-recovery/` | **DONE** — Java+JS verified, 17 tests each, 7 inputs, panic mode (FOLLOW sync + token discard) + phrase-level insertion, one run reports every error with position, termination argument, tutorial + DECISIONS + glossary |
| 28 | Grammar Analysis Tool  *(Project 2)* | Proves one program that reads any CFG and emits: ordered terminals/nonterminals, nullable, FIRST, FOLLOW, left-factored grammar, left-recursion-free grammar, and the LL(1) verdict. | `28-grammar-tool-project2/` | **DONE** — CSE 340 Project 2: Java+JS verified, 18 tests each, 3 grammars, 7-section report composing Modules 20–27, guarded Paull, recursion-then-factoring fix order, "does the fix work?" check, tutorial + DECISIONS + glossary |
| 29 | LL(k), LL(*), and Backtracking RD | Proves where k>1 lookahead and ANTLR-style adaptive prediction help, and what they cost. | `29-llk-adaptive/` | **DONE** — Java+JS verified, 14 tests each, 3 grammars, FIRST_k fixed point, minimal-k search (label grammar = LL(2), equiv = no k), parse-tree-counting backtracker showing 2ⁿ vs linear work, LL(*) explained, tutorial + DECISIONS + glossary |

### Part III — COMPLETE ✅

> Modules 19–29 done. The whole top-down toolkit: recursive descent (19), the
> predictive condition (20), FIRST/nullable (21) and FOLLOW (22) as least fixed
> points, left-recursion elimination (23) and left factoring (24), the LL(1)
> table (25), the table-driven parser (26), error recovery (27), the Grammar
> Analysis Tool = CSE 340 Project 2 (28), and the LL(k)/LL(*)/backtracking
> spectrum (29). Every module: Java + JS byte-identical to a committed golden,
> both test suites green.

### Part IV — Bottom-Up Parsing

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 30 | Handles, Shift-Reduce & Viable Prefixes | Proves every right-sentential form has a unique handle and the parse stack always holds a viable prefix. | `30-shift-reduce-handles/` | **DONE** — Java+JS verified, 14 tests each, 2 grammars/4 inputs, shift-reduce trace with handles, reductions reversed = rightmost derivation (checked), left-recursive grammar parsed directly, shift-reduce conflict detected on `E→E+E`, tutorial + DECISIONS + glossary |
| 31 | LR(0) Items & the Canonical Collection | Proves item sets are the states of a DFA that recognizes viable prefixes; builds CLOSURE and GOTO. | `31-lr0-items/` | Not started |
| 32 | SLR(1) Parsing | Proves FOLLOW-based reduce decisions, and shows a grammar that is LR(0)-broken but SLR-fine and one that is the reverse. | `32-slr1/` | Not started |
| 33 | Canonical LR(1) | Proves lookahead carried inside items removes SLR's spurious reductions, at the cost of state explosion. | `33-lr1/` | Not started |
| 34 | LALR(1) | Proves merging LR(1) states by core shrinks the table, and exhibits the reduce-reduce conflict merging can introduce. | `34-lalr1/` | Not started |
| 35 | Conflicts, Precedence Declarations & yacc/Bison | Proves `%left` / `%right` / `%prec` resolve shift-reduce conflicts deterministically, using the dangling-else grammar. | `35-conflicts-precedence/` | Not started |
| 36 | An SLR/LALR Table Generator | Proves a program that emits ACTION/GOTO tables from a grammar plus a driver that parses with them. | `36-lr-table-generator/` | Not started |

### Part V — Syntax-Directed Translation & AST Construction  · *→ CSE 340 Project 3*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 37 | Attribute Grammars: Synthesized & Inherited | Proves attributes flow strictly up (S-attributed) or up-and-down (L-attributed), via the dependency graph. | `37-attribute-grammars/` | Not started |
| 38 | SDDs vs. Translation Schemes | Proves where an action must sit in a production to evaluate L-attributed definitions during LL and LR parsing. | `38-sdd-translation-schemes/` | Not started |
| 39 | Building Abstract Syntax Trees | Proves the AST drops punctuation and chain productions, and pins down the parse-tree → AST mapping. | `39-building-asts/` | Not started |
| 40 | Ordered Semantic-Error Reporting  *(Project 3a)* | Proves a parser that reports semantic errors (undeclared, redeclared, arity, type) in a defined priority order with exact line/column, modeled on the CSE 340 "poly" project. | `40-semantic-errors-project3/` | Not started |
| 41 | Evaluating the Parsed Language  *(Project 3b)* | Proves a tree-walk over the AST executes `INPUT` / `EXECUTE`-style statements and produces the graded output. | `41-tree-walk-execution/` | Not started |

### Part VI — Static Semantics: Symbols, Scope, Types  · *→ CSE 340 Project 4*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 42 | Symbol Tables & Scope Resolution | Proves a scope stack with hash chains resolves names, and that static vs. dynamic scoping give different answers for the same program. | `42-symbol-tables-scope/` | Not started |
| 43 | Type Systems, Equivalence & Checking | Proves structural vs. name equivalence differ, and implements the `Γ ⊢ e : τ` checking judgment. | `43-type-checking/` | Not started |
| 44 | Type Inference from Usage  *(Project 4)* | Proves operator constraints induce types for undeclared variables and that conflicts are reportable, modeled on the CSE 340 type-inference project. | `44-type-inference-project4/` | Not started |

### Part VI-b — Runtime Environments (parsing's downstream contract)

Short bridge: what the parser's output is ultimately *for*. Kept minimal — the
neighboring compiler course owns full code generation.

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| 45 | Runtime Environments & Activation Records | Proves the AST's call/return structure maps onto a stack of activation records with a fixed frame layout. | `45-activation-records/` | Not started |
| 46 | Parameter-Passing Modes | Proves call-by-value, -reference, -value-result, and -name give different results for the same syntax tree, via a swap-and-alias test. | `46-parameter-passing/` | Not started |
| 47 | Scope Implementation: Static/Dynamic Links & Displays | Proves nested-function variable access resolves through static links (or a display), distinct from the dynamic call chain. | `47-scope-links-displays/` | Not started |
| 48 | Storage: Stack, Heap & Garbage Collection | Proves lifetime, not scope, decides stack vs. heap, and implements mark-sweep and reference counting over the interpreter's objects. | `48-storage-and-gc/` | Not started |

---

## Appendix (optional — not part of the committed course)

Built only on later request. Numbering `X1–X16` to keep it clear these are not
in the main sequence.

### Appendix A — Expert Extensions: Everything Else in Parsing

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| X1 | PEG: Parsing Expression Grammars | Proves ordered choice plus unlimited lookahead yields an unambiguous formalism, and pins the exact inputs where a PEG and the "same" CFG disagree. | `appendix/x01-peg/` | Not started |
| X2 | Packrat Parsing | Proves memoizing `(rule, position)` makes PEG linear time, at linear space, and handles left recursion via Warth's seed-growing. | `appendix/x02-packrat/` | Not started |
| X3 | Parser Combinators | Proves parsers compose as first-class values through an applicative/monadic interface, in both Java and JS. | `appendix/x03-parser-combinators/` | Not started |
| X4 | Pratt Parsing / Precedence Climbing | Proves binding-power numbers with null/left denotations replace an entire tower of precedence rules. | `appendix/x04-pratt-parsing/` | Not started |
| X5 | Operator-Precedence Parsing | Proves the classical precedence-relation table method (⋖ ≐ ⋗) parses expression grammars without their nonterminals. | `appendix/x05-operator-precedence/` | Not started |
| X6 | Earley Parsing | Proves any CFG parses in O(n³) (O(n²) unambiguous, O(n) bounded-lookahead) via scan/predict/complete, and builds the shared packed parse forest. | `appendix/x06-earley/` | Not started |
| X7 | CYK Parsing | Proves parsing is dynamic programming once the grammar is in Chomsky Normal Form, via the recognition table. | `appendix/x07-cyk/` | Not started |
| X8 | GLR / GLL Parsing | Proves a graph-structured stack lets a table parser handle every CFG, including ambiguous ones, returning a parse forest. | `appendix/x08-glr-gll/` | Not started |
| X9 | Advanced Error Recovery | Proves Burke–Fisher repair, error productions, and least-cost repair beat panic mode, and defines how to measure recovery quality. | `appendix/x09-advanced-error-recovery/` | Not started |
| X10 | Indentation-Sensitive & Layout Parsing | Proves INDENT/DEDENT synthesis and Haskell-style layout turn 2-D structure into an ordinary token stream. | `appendix/x10-indentation-layout/` | Not started |
| X11 | Scannerless & Context-Aware Parsing | Proves merging lexer and parser handles embedded languages and the C "typedef-name" lexer-feedback hack. | `appendix/x11-scannerless-context/` | Not started |
| X12 | Incremental Parsing | Proves reparse-on-keystroke by reusing untouched subtrees, following tree-sitter's method. | `appendix/x12-incremental-parsing/` | Not started |
| X13 | Unicode, Encodings & the Input Layer | Proves the grapheme / code point / code unit choice changes tokenization, and covers BOM, normalization, and bidi. | `appendix/x13-unicode-input-layer/` | Not started |
| X14 | Ambiguity Detection & Grammar Engineering | Proves grammar ambiguity is undecidable in general, and that sentence-generation detectors and disambiguation filters are the practical response. | `appendix/x14-ambiguity-detection/` | Not started |

### Appendix B — Capstone: Build the Toolchain

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| X15 | A Parser Generator (mini-yacc / mini-ANTLR) | Proves an EBNF grammar in yields working recursive-descent *and* LALR parser source out, in both Java and JS. | `appendix/x15-parser-generator/` | Not started |
| X16 | A Grammar Workbench | Proves one stored grammar drives railroad diagrams, FIRST/FOLLOW tables, LL/LR conflict reports, and a live parse-tree viewer. | `appendix/x16-grammar-workbench/` | Not started |

---

## Recommended Stopping Points

| Your goal | Stop after |
|-----------|-----------|
| Read a language spec fluently: grammars, BNF/EBNF, ambiguity, the hierarchy | **Module 8** |
| Pass the equivalent of CSE 340 Project 1 (a real lexer) | **Module 18** |
| Pass the equivalent of CSE 340 Project 2 (grammar analysis + LL(1)) | **Module 28** |
| Hand-write production parsers for your own languages | **Module 29** |
| Understand LR, LALR, and yacc/Bison output | **Module 36** |
| Pass the equivalent of CSE 340 Projects 3–4 (SDT, semantic errors, types) | **Module 44** — *the full CSE 340 parsing core* |
| Understand what the parse tree feeds (frames, scope, GC) | **Module 48** — *end of committed course* |
| The modern practitioner toolkit (PEG, packrat, combinators, Pratt) | **Appendix X4** |
| The exotic general-CFG algorithms (Earley, CYK, GLR) | **Appendix X8** |
| Build your own parser generator + grammar workbench | **Appendix X16** |

---

## Prerequisites Layer (seeded before Module 1, grown as needed)

Root `prerequisites/index.html` linking one page per assumed primitive. Modules
link back with short tag-style links and never re-teach inline.

1. `sets-relations-fixed-points.html` — sets, membership, relations, closures, least fixed points (the shape of FIRST/FOLLOW/nullable)
2. `functions-and-notation.html` — total vs. partial functions, and the notation wall: Σ, ε, ⇒, ⊢, ⊆
3. `induction-and-recursion.html` — structural induction, the call stack, base cases, stack overflow
4. `trees.html` — nodes, roots, leaves, pre/post-order, tree equality
5. `automata-basics.html` — states, transitions, accept states, determinism, ε
6. `grammar-notation-quickref.html` — terminal, nonterminal, production, sentential form, `::=`
7. `hash-tables-and-scoping.html` — hash maps, chaining, the stack-of-scopes pattern
8. `the-cursor-pattern.html` — `peek` / `advance` / `mark` / `reset`, the shared input idiom
9. `java-and-js-setup.html` — exact toolchain, how to run and test one module, the ~20-line `Assert` helper used instead of a framework

---

## Tools / Architecture Target

- **Two languages, always in parallel.** Java 17+ and JavaScript (ES2022,
  Node 20+). Every module ships both; each tutorial shows them side by side and
  flags where the language forces a different choice (`char` vs. UTF-16 code
  unit, checked exceptions, `switch` patterns, `BigInt`, no operator overloads).
- **Zero third-party libraries.** Java: `javac` / `java`, no JUnit — a small
  `Assert` helper from the prerequisites layer. JavaScript: `node` with the
  built-in `node:test` only.
- **No build tool.** Each module directory is self-contained: `java/`, `js/`,
  `fixtures/` (shared inputs), `expected/` (golden output), `run.md` (exact
  PowerShell commands, bash equivalents alongside).
- **CSE 340 test discipline.** Every project module has an input-file / expected-
  output-file test set, and the tutorial's "Run It" section pastes the *real*
  produced output, byte for byte.
- **Platform.** Developed and verified on Windows 11 / PowerShell. Pure CLI.
- **Explicitly out of scope:** machine code generation, register allocation,
  and instruction selection (the neighboring `../module_*` compiler course owns
  those). Part VI-b covers runtime *concepts* (frames, parameter passing, GC) at
  interpreter level only. Also out: production tools (ANTLR, Bison, tree-sitter)
  except as "compare your output to theirs" sidebars; natural-language /
  statistical parsing; full Unicode normalization and locale collation; parser
  performance tuning past algorithmic complexity.

---

## Design System (every tutorial.html)

One reused system for visual continuity, adapted from the attached
`foster-parenting-course.html`:

- Light "engineering paper" background with a faint grid.
- `Space Grotesk` display, `Newsreader` body, `IBM Plex Mono` code — Google
  Fonts with full fallback stacks.
- Fixed structural blocks: **THE PICTURE** (the analogy — the required section
  01), **THE MECHANISM** (the formal definition / algorithm, never blended into
  prose), and colored callouts (`watch` / `brain` / `crazy`).
- **One accent color per module** over the shared `--pen` / `--stamp` base.
- Required sections in order: `00` Prerequisites · `01` The Big Analogy · `02`
  What Happens, Step by Step · `03` State Map/Diagram · `04` The Code, Explained
  · `05` Limits (can't-do vs. can-do) · `06` Run It (tested commands + real
  output + troubleshooting) · `07` Brain Exercise · `08` There Are No Dumb
  Questions · `09` What's Next. Footer links prev/next.

Theme locked: **attached light template** (Space Grotesk / Newsreader / IBM Plex
Mono on engineering-paper).

---

## Phase 0 Decisions (locked 2026-08-30)

| Question | Decision |
|----------|----------|
| Count | Build straight through **Module 48** (Parts I–VI + VI-b). No stop at Part boundaries. Appendix A/B only on later request. |
| Runtime topics | **In**, as Part VI-b — interpreter-level frames / parameter passing / GC, 4 modules. |
| Theme | Attached light template. |
| First build | **Module 1**, then strictly in order. |
| Cadence | One module fully finished (Java + JS code → verified real output → DECISIONS.md → tutorial.html → GLOSSARY append) before the next. Continuous — no approval gate between Parts. A minimal `prerequisites/` layer (index + the pages Module 1 links, + the shared `Assert` helper) is built first; remaining prereq pages are added as later modules first reference them. |
