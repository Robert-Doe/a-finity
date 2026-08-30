# Everything Parsing — Glossary

Every term, file, and named constant the course introduces, alphabetical.
Each entry is tagged with the module it is **first seen** in. This file is only
ever appended to or inserted into — never rewritten.

---

### ambiguity (of a grammar)
A grammar is ambiguous if some string has two or more distinct parse trees
(equivalently, two leftmost derivations). Deciding ambiguity for an arbitrary
CFG is undecidable. Some languages are *inherently ambiguous* — every grammar
for them is ambiguous. *First seen: Module 6.*

### Ambiguity (class)
`Ambiguity.java` / `ambiguity.mjs`: `allTrees(g, w)` enumerates every parse tree
for `w` via a pruned leftmost-derivation search; `isAmbiguousFor`,
`smallestAmbiguousString(g, maxLen)` (a bounded witness finder). *First seen:
Module 6.*

### amortised O(1)
Any single operation may be costly, but any long run of them averages to
constant time each. Module 9's `advance()`: mostly `forward++`, plus one buffer
load per `halfSize` chars, spread over the cheap calls. *First seen: Module 9.*

### alphabet (Σ)
A finite, non-empty set of symbols. In Module 2's code a `Set<Character>`; in
grammars, the set of terminals. *First seen: Module 2.*

### associativity
For a binary operator, which grouping applies to `a op b op c` when the grammar
doesn't say: left (`(a op b) op c`) or right (`a op (b op c)`). Encoded by the
recursion side of the production — `E -> E + T` is left, `E -> T + E` is right.
*First seen: Module 6; formalised in Module 20.*

### ABNF
Augmented BNF (RFC 5234) — the IETF's grammar notation for HTTP, email, URI,
DNS. `/` for alternation, `*rule` / `1*rule` / `2*4rule` for repetition counts,
`[x]` optional, case-insensitive `"..."`, `%x41` byte values. Same power as BNF.
*First seen: Module 8.*

### Assert / assert.mjs
The course's entire test framework: ~40 lines, no JUnit, no test runner.
`that(cond, label)`, `equals(actual, expected, label)`, `summary()`. Canonical
copy in `prerequisites/assert/`; a byte-identical copy sits in every module's
`java/` and `js/`. *First seen: Module 1.*

### checkSyntax
The function that decides well-formedness of an RPN expression by simulating the
operand stack's **depth** only — it never parses a number or does arithmetic.
Returns `(ok, message, col)`. *First seen: Module 1.*

### column (1-based)
A token's position reported as "characters from the start of the line, counting
from 1." Matches what editors and compilers show. Computed once, as `start + 1`
in `tokenize`. `col == 0` is the sentinel for "this error has no single
position." *First seen: Module 1.*

### Buffer (class) / two-buffer scheme / sentinel
`Buffer.java` / `buffer.mjs`: the Dragon Book §3.2 input scheme — two halves of
`halfSize` chars filled on demand, a `SENTINEL` (`-2`) after each half so the
inner loop tests one value that means "reload" or (as `EOF`, `-1`) "stop".
`peek` / `advance` / `mark` / `retract` / `lexeme`. Lexeme must fit one half.
*First seen: Module 9.*

### block comment / line comment / nested comment
**Line comment** — `//` to end of line; regular. **Block comment** — `/* … */`;
regular *only* if it does not nest (C-style). **Nested comment** — `/* /* … */ */`
where inner pairs must balance; **not regular** (same shape as `aⁿbⁿ`, Module 7),
so it needs a **depth counter**: `depth++` on `/*`, `depth--` on `*/`, stop at
`depth == 0`. `skipBlockComment` in `PracticalLexer`. Rust/Swift/Kotlin nest;
C/Java do not. *First seen: Module 18.*

### depth counter
The single `int depth` that makes nested comments tractable — one unbounded
counter is a pushdown-automaton move, the minimum power a finite automaton
lacks. See [[block comment / line comment / nested comment]]. *First seen: Module 18.*

### BNF / EBNF / ISO-EBNF
**BNF** — Backus–Naur Form: bare productions, `::=` or `->`, `|`, recursion for
repetition. **EBNF** — Extended BNF: adds `?` `*` `+` `( )` as shorthand (this
course's `.ebnf` files). **ISO-EBNF** (ISO 14977) — `{ x }` = zero or more,
`[ x ]` = optional. All inter-convertible; Module 8's `toBnfText` desugars EBNF
to BNF. *First seen: Module 8.*

### Chomsky hierarchy
Four nested language classes: Type 3 regular ⊊ Type 2 context-free ⊊ Type 1
context-sensitive ⊊ Type 0 recursively enumerable, recognised respectively by a
finite automaton, a pushdown automaton, a linear-bounded automaton, a Turing
machine. Witnesses separating the bottom three: `a*`, `aⁿbⁿ`, `aⁿbⁿcⁿ`. *First
seen: Module 7.*

### concatenation (of languages)
`L₁ · L₂ = { xy : x ∈ L₁, y ∈ L₂ }` — the glued cartesian product. `∅` is the
annihilator (`∅ · L = ∅`); `{ε}` is the identity (`{ε} · L = L`). Associative,
not commutative. *First seen: Module 2.*

### closure property
"Applying this operation to members of a class yields another member of the
class." Finite languages are closed under union, concatenation, and power; the
regular languages are closed under all of those plus star, intersection, and
complement. *First seen: Module 2.*

### context-free grammar (CFG)
A 4-tuple (N, Σ, P, S): nonterminals, terminals, productions, start symbol. Each
production rewrites a *single* nonterminal (that's the "context-free" part).
Generates exactly the context-free languages. *First seen: Module 4.*

### derivation / derivation step (⇒, ⇒*)
`α ⇒ β`: β is α with one nonterminal replaced by the right-hand side of one of
its productions. `⇒*` is zero or more such steps. `L(G) = { w ∈ Σ* : S ⇒* w }`.
*First seen: Module 4.*

### Derivation (class) / derives / enumerate
`Derivation.java` / `derivation.mjs`: `leftmostDerivation` (apply chosen
productions to the leftmost nonterminal), `enumerate` (all terminal strings ≤ a
length bound, breadth-first), `derives` (bounded BFS membership test).
*First seen: Module 4.*

### Dyck language
The language of balanced bracket strings, generated by `S -> ( S ) S | epsilon`.
Module 4's second fixture. *First seen: Module 4.*

### empty language (∅) vs. empty string (ε)
`∅` is a language with **no** strings (`size 0`). `{ε}` is a language with
**one** string, the zero-length string `""` (`size 1`). They behave oppositely
under concatenation — `∅` annihilates, `{ε}` is the identity. *First seen:
Module 2.*

### free monoid
Σ* under concatenation: an associative operation with identity `ε`, no inverses,
not commutative. The abstract structure every string type is an instance of.
*First seen: Module 2 (mentioned).*

### Kleene star (L*)
`L* = L⁰ ∪ L¹ ∪ L² ∪ …` — zero or more concatenated copies; always contains
`ε`. The only basic language operation whose result can be infinite, so the
code computes it up to a length bound via breadth-first closure. *First seen:
Module 2.*

### Language (class)
`Language.java` / `language.mjs`: a finite set of strings in canonical order
(length, then lexicographic), with `union`, `concat`, `power`, `star`,
`intersect`, `minus`, and `sigmaStar`. *First seen: Module 2.*

### power (Lⁿ)
`L` concatenated with itself `n` times; `L⁰ = {ε}` for every `L` (including
`∅⁰ = {ε}`). *First seen: Module 2.*

### Sigma star (Σ*)
The set of all strings over Σ, including `ε`; always infinite for non-empty Σ.
Computed here bounded, as the Kleene star of the one-character languages.
*First seen: Module 2.*

### string (formal)
A finite ordered sequence of symbols from an alphabet. Length may be 0 (`ε`).
*First seen: Module 2.*

### union (of languages)
Set union: `L₁ ∪ L₂` = strings in either. Commutative and associative — which is
why `a|b|c` needs no parentheses. *First seen: Module 2.*

### DFA (deterministic finite automaton)
`(Q, Σ, δ, q₀, F)` with `δ : Q × Σ → Q` **total**. Decides membership in one
left-to-right pass, O(n), with only the current state as memory. Accepts exactly
a regular language. `Dfa.java` / `dfa.mjs`. *First seen: Module 10.*

### dead state (trap state)
A non-accepting state that self-loops on every symbol. `Dfa.parse` routes every
undefined `(state, symbol)` pair here so `δ` is total. *First seen: Module 10.*

### epsilon-closure (ε-closure)
Given a set of NFA states, all states reachable from it by ε-transitions alone
(itself included). Computed by worklist reachability. Run before the first input
symbol and after every symbol during NFA simulation. *First seen: Module 11.*

### epsilon elimination
Rewriting an ε-NFA into an equivalent ε-free NFA:
`δ'(q, a) = εClosure(δ(εClosure(q), a))`, and `q` becomes accepting iff its
ε-closure contained an accepting state. Proves ε adds no power. `Nfa.removeEpsilon`.
*First seen: Module 11.*

### Myhill–Nerode theorem
The states of the minimal DFA for a language correspond one-to-one with the
language's *right-language equivalence classes* (a property of the language, not
any machine). Consequences: the minimal DFA is unique up to renaming, its state
count is exactly the number of classes, and it gives a decision procedure for
regular-language equivalence. *First seen: Module 14.*

### minimization (of a DFA)
Merging equivalent states (same right language) into one. Two algorithms:
partition refinement (Moore — split blocks by transition signature to a fixed
point) and table filling (mark distinguishable pairs to a fixed point).
`Minimize.java` / `minimize.mjs`. *First seen: Module 14.*

### NFA (nondeterministic finite automaton)
Like a DFA but `δ(q, a)` is a *set* of states (possibly empty), and ε-transitions
are allowed. Accepts if *some* path ends accepting. Simulated by tracking the set
of possible states in lockstep. Same power as a DFA. `Nfa.java` / `nfa.mjs`.
*First seen: Module 11.*

### desugaring
Rewriting a convenience form into core forms before further processing.
Module 3 desugars `r+ → r r*` and `r? → r | ε` during parsing, so the AST only
ever has six node kinds. *First seen: Module 3.*

### disambiguation
Removing a grammar's ambiguity without changing its language. The two standard
rewrites: a nonterminal tier per precedence level, and left/right recursion per
operator for associativity. Not always possible (inherently ambiguous
languages); never automatic. *First seen: Module 6.*

### dangling else
The ambiguity in `S -> if C then S | if C then S else S`: in
`if C then if C then a else b`, the `else` can attach to either `if`. Fixed by
convention ("nearest unmatched `then`") — Module 35. *First seen: Module 6.*

### dynamic semantics
The meaning of a program in terms of what happens when it runs (values,
effects, faults like overflow or divide-by-zero). Contrast **static
semantics**. *First seen: Module 1.*

### evaluate
The function that runs a *syntactically valid* RPN expression on a value stack
and returns its number, or the one defined semantic error (`division by zero`)
with a column. *First seen: Module 1.*

### expected/
Per-module directory holding committed **golden output** — the exact bytes a
demo program produces. Tutorials paste from here verbatim; tests and `run.md`
diff against it. *First seen: Module 1.*

### fixtures/
Per-module directory of input files read by *both* the Java and the JavaScript
build, so the two are exercised on identical data. *First seen: Module 1.*

### golden output
A committed reference copy of a program's exact output, used to detect any
change in behavior. *First seen: Module 1.*

### FIRST set
FIRST(a) = the set of terminals that can begin a string derived from the symbol
string `a`, plus the marker `epsilon` if `a` is [[nullable]]. Computed by
iterating to a fixed point (Module 21). Drives the parser's choice of
production. *First seen: Module 20.*

### FOLLOW set
FOLLOW(A) = the set of terminals that can appear immediately after nonterminal
`A` in some sentential form, plus `$` if `A` can be last. Needed to choose a
nullable production: what follows `A` is what the parser sees when `A` produces
nothing. Fixed-point computation in Module 22. *First seen: Module 20.*

### nullable
A symbol string is nullable if it can derive the empty string. `epsilon in
FIRST(a)` is the test. `A -> epsilon` makes `A` nullable directly; `A -> B C` if
both `B` and `C` are nullable. Its own least-fixed-point computation in
Module 21. *First seen: Module 20.*

### left recursion elimination
Rewriting a grammar so no nonterminal can derive a form starting with itself,
without changing the language. **Direct**: `A -> A a | b` becomes `A -> b A'`,
`A' -> a A' | epsilon`. **Indirect**: Paull's algorithm — order the
nonterminals, substitute lower ones into higher ones, then remove the direct
recursion. Required before top-down parsing (Module 19). Trades left
associativity for parsability. `LeftRec` in Module 23. *First seen: Module 23.*

### Grammar Analysis Tool (Project 2)
One program (`GrammarTool`, Module 28) that reads any CFG and reports:
symbols in appearance order, [[nullable]], [[FIRST set]]s, [[FOLLOW set]]s, a
[[left recursion elimination|left-recursion-free]] version, a [[left factoring|
left-factored]] version, and the [[LL(1)]] verdict + table (or the conflicts and
whether the transforms fix them). Pure composition of Modules 20–27; models CSE
340 Project 2. *First seen: Module 28.*

### handle
In a right-sentential form, the substring that the last step of a rightmost
derivation expanded — the production RHS to reduce next in a bottom-up parse.
It is always at the **top of the stack** when a shift-reduce parser reduces.
An unambiguous grammar gives every right-sentential form a unique handle.
*First seen: Module 30.*

### shift-reduce parsing
Bottom-up parsing with a stack of grammar symbols and two moves: **shift**
(push the next input terminal) and **reduce** `A -> b` (the top `|b|` stack
symbols are `b`, a [[handle]]; pop them, push `A`). Accept when the stack is
`[start]` and the input is consumed. The reductions reversed are a rightmost
derivation. `ShiftReduce` in Module 30. *First seen: Module 30.*

### viable prefix
Any string of grammar symbols that can appear on a shift-reduce parser's stack
during a valid parse — equivalently, a prefix of a right-sentential form that
does not extend past the right end of its handle. The set of viable prefixes of
a grammar is **regular**; the DFA that recognises it is the LR parser's engine
(Module 31). *First seen: Module 30.*

### shift-reduce conflict / reduce-reduce conflict
A state from which two parser moves are both valid: **shift-reduce** = shifting
the next token *and* reducing by some production; **reduce-reduce** = two
different productions could reduce the current stack top. The grammar (or the
parser class) can't decide with the available lookahead. Resolved by precedence
declarations in Module 35. *First seen: Module 30.*

### left factoring
Rewriting a rule so no two alternatives share a first symbol:
`A -> a b1 | a b2 | rest` becomes `A -> a A' | rest`, `A' -> b1 | b2` (empty
`bi` -> `A' -> epsilon`). Factor the longest shared prefix, repeat to a fixed
point. Language-preserving. Necessary but **not sufficient** for [[LL(1)]] — a
FOLLOW conflict (dangling else) survives factoring. `LeftFactor` in Module 24.
*First seen: Module 24.*

### Paull's algorithm
The standard procedure for removing indirect left recursion. For nonterminals
`A1..An` in a fixed order: for each `i`, replace every `Ai -> Aj g` (`j < i`)
with `Ai -> d g` for each `Aj -> d`, then eliminate direct left recursion in
`Ai`. Terminates; the result is not left-recursive. Can multiply productions.
*First seen: Module 23.*

### phrase-level recovery
Parser error recovery that makes a local edit — most commonly inserting a
terminal the grammar demanded but the input lacks (a missing `)`, `;`), then
continuing without discarding input. Used for a terminal-on-top mismatch;
contrast [[panic-mode recovery]] which discards input. `RecoveringParser` in
Module 27. *First seen: Module 27.*

### table-driven predictive parsing
Parsing with an explicit stack + the [[LL(1) parsing table]] + a loop, no
recursion. Stack starts `[$ , start]`; a nonterminal on top is expanded via
`M[A][lookahead]` (pop, push RHS reversed); a terminal on top is matched against
the lookahead; `$` over `$` accepts. Produces the identical leftmost derivation
that recursive descent (Module 19) does — the data stack *is* the call stack.
`TableParser` in Module 26. *First seen: Module 26.*

### LL(1) parsing table
A grid `M[A][t]` — nonterminal `A` by lookahead terminal `t` (plus `$`) — whose
cell names the one production to apply. Built by placing each production `A -> a`
into `M[A][t]` for every `t` in `PREDICT(A -> a)`. Single-valued in every cell
iff the grammar is [[LL(1)]]; a cell with two productions is a conflict witness.
A blank cell is a syntax error. `LL1Table` in Module 25. *First seen: Module 25.*

### LL(k)
Predictive parsing with **k** tokens of lookahead. A grammar is LL(k) when every
multi-production nonterminal has pairwise-disjoint FIRST_k sets on its
alternatives (FOLLOW_k folded in for nullable ones). LL(k+1) is strictly more
powerful than LL(k). `FirstK` in Module 29. *First seen: Module 29.*

### FIRST_k
FIRST_k(a) = every terminal string of length <= k that can begin a derivation
from `a`. FIRST_1 is the ordinary [[FIRST set]]. Computed by the same
fixed-point iteration, truncating joined prefixes to k tokens.
*First seen: Module 29.*

### LL(*) / adaptive LL(*)
Lookahead that is **regular but unbounded** — a DFA over the token stream reads
as many tokens as needed to pick an alternative, rather than a fixed k. ANTLR 4's
core (ALL(*)), falling back to full backtracking only for genuine ambiguity.
*First seen: Module 29.*

### backtracking recursive descent
Recursive descent that, on a wrong alternative, restores the cursor and tries
the next. Without memoization the work is exponential when alternatives overlap
(the same (nonterminal, position) is re-solved on every path). Memoizing that
pair is [[packrat parsing]] and makes it linear. `Backtrack` in Module 29.
*First seen: Module 29.*

### least fixed point
The smallest set (or tuple of sets) that a monotone rule system maps to itself.
Reached by starting from empty and applying the rules until a full pass adds
nothing — monotone (only adds) + bounded (finite alphabet) guarantees it stops.
[[nullable]], [[FIRST set]], and [[FOLLOW set]] are all least fixed points; a
*larger* fixed point would assert derivations the grammar cannot actually
produce. `FirstSets` in Module 21. *First seen: Module 21.*

### PREDICT set
PREDICT(A -> a) = FIRST(a) if `a` is not nullable, else
(FIRST(a) \ {epsilon}) U FOLLOW(A). The tokens on which a predictive parser
picks this production. Always a set of real terminals (may include `$`), never
`epsilon`. *First seen: Module 20.*

### LL(1)
A grammar is LL(1) iff, for every nonterminal, the [[PREDICT set]]s of its
productions are pairwise disjoint — one lookahead token always determines the
production, no backtracking. **L**eft-to-right scan, **L**eftmost derivation,
**1** token of lookahead. Ambiguous and left-recursive grammars are never
LL(1); many others become LL(1) after [[left recursion]] elimination (Module 23)
and left factoring (Module 24). *First seen: Module 20.*

### end-of-input marker ($)
A synthetic terminal appended to the token stream, and an element of FOLLOW
sets, marking "nothing more comes". `$ in FOLLOW(A)` means `A` can end the
input. The parser driver supplies it at EOF (Module 26). *First seen: Module 20.*

### Ebnf (class)
`Ebnf.java` / `ebnf.mjs`: `parse(src)` → an EBNF AST; `toBnfText(g)` desugars
`? * + ( )` into fresh nonterminals over pure BNF; `accepts(g, tokens)` matches
a token list exactly against the EBNF; `iso` / `isoRule` render ISO-EBNF.
*First seen: Module 8.*

### epsilon production
A production with an empty right-hand side (`S -> epsilon` or `S ->`). In the
code, `rhs.isEmpty()`. There is no `epsilon` symbol in Σ — it is file-format
sugar only. *First seen: Module 4.*

### Grammar (class) / Production
`Grammar.java` / `grammar.mjs`: a CFG loaded from a `.grammar` file. One
`Production` (index, lhs, rhs-list) per alternative — the `|` is expanded at
load time. Nonterminals = symbols with a rule; terminals = everything else.
*First seen: Module 4.*

### L(G) / language of a grammar
The set of all terminal strings derivable from the start symbol:
`{ w ∈ Σ* : S ⇒* w }`. *First seen: Module 4.*

### leftmost derivation
A derivation in which every step expands the leftmost nonterminal of the current
sentential form. Every derivable string has exactly one leftmost derivation per
parse tree. Contrast rightmost (Module 5). *First seen: Module 4.*

### left recursion
A grammar rule that can derive a sentential form starting with itself —
directly (`E -> E + T`) or indirectly. **Fatal to top-down parsing**: the
method for `E` would call `E` with the cursor unmoved, recursing forever with
no progress. Removed by Paull's algorithm (Module 23). Contrast the harmless
*right* recursion `E' -> + T E'`, where a token is consumed before the
recursive call. *First seen: Module 19.*

### lexeme
The raw substring of source that forms one token — e.g. `"12"`, `"/"`. The
token is the lexeme plus metadata (here, its column). *First seen: Module 1.*

### recursive-descent parsing
Top-down parsing by hand: **one function per nonterminal**, whose body calls the
functions for the symbols on its right-hand side. The current token picks the
alternative; no backtracking (that's *predictive* recursive descent). The chain
of active calls at any moment is a [[leftmost derivation]]. Needs a grammar with
no [[left recursion]] and disjoint per-alternative FIRST sets. `RecursiveDescent`
in Module 19; used in real compilers (GCC, Clang, V8, rustc). *First seen: Module 19.*

### predictive parsing
Parsing that commits to one production per (nonterminal, lookahead) with no
backtracking. Possible exactly when a rule's alternatives have pairwise-disjoint
FIRST sets (plus a FOLLOW side-condition for nullable alternatives) — the
**predictive parsing condition**, formalised in Module 20. In Module 19 the
condition is checked by inspection. *First seen: Module 19.*

### concrete syntax tree (parse tree) vs. abstract syntax tree (AST)
The **concrete** tree has a node for every grammar symbol used, including
punctuation, `epsilon` leaves, and chain productions (`T -> F`). The **AST**
keeps only what later stages need — operators and operands, no parens, no
primes. Module 19 builds the concrete tree; the parse-tree → AST mapping is
Module 39. *First seen: Module 19.*

### reserved-word trick
The way real lexers handle keywords: scan the maximal identifier
`[A-Za-z_][A-Za-z0-9_]*`, then look the text up in a `KEYWORDS` set —
`let` → `KW_LET`, `lettuce` → `ID`. One identifier scanner + a hash lookup,
versus one DFA per keyword and a rebuild on every addition. Equivalent in effect
to [[token priority]]. `identifierOrKeyword` in `PracticalLexer`.
*First seen: Module 18.*

### string escape
Inside a `"…"` literal, a `\` plus one selector stands for a character that is
awkward or impossible to write literally: `\n` newline, `\t` tab, `\"` quote,
`\\` backslash, `\uXXXX` a code point from exactly four hex digits. The lexer
**decodes** these into the token's value (not the source spelling), because
downstream stages want the meaning. Unknown selector → an error, scan continues.
*First seen: Module 18.*

### maximal munch (longest match)
The lexer rule: at each position, the token is the **longest** prefix that any
pattern matches. Mechanism: run the DFA, remember the last position it was
accepting, back up to it. `iffy` → `ID`, not `if` + `fy`. *First seen: Module 1
(mentioned); built in Module 15.*

### token priority
The tie-breaker: when two patterns match the same longest length, the one
**declared first** wins. This is how keywords are reserved. In code: only update
the best match on a *strictly* longer length. *First seen: Module 15.*

### Lexer (class)
`Lexer.java` / `lexer.mjs`: `Rule` (name + pattern → minimal DFA),
`longestAccept` (the last-accept mark), `nextToken` (the two rules), `tokenize`.
*First seen: Module 15.*

### SpecLexer (class) / token spec / combined DFA
`SpecLexer.java` / `speclexer.mjs` — CSE 340 Project 1. Loads a `NAME regex`
spec (priority = file order, `%skip NAME` for trivia), unions every pattern's
NFA under a fresh start with a tag per accept state, determinizes into one
**combined DFA** whose states carry the winning token, and scans in O(input).
*First seen: Module 16.*

### "epsilon IS NOOOOOT A TOKEN"
CSE 340's load-time rejection of a token pattern that matches the empty string
(`εClosure({start}) ∩ accept ≠ ∅`). Such a pattern makes the scanner spin
forever. *First seen: Module 16.*

### nonterminal
A grammar symbol that has at least one production — a "shape" name, not a real
token. Written in capitals by convention. Contrast **terminal**. *First seen:
Module 4.*

### operand stack
The last-in-first-out store an RPN evaluator pushes operands onto and pops
operands from. The syntax checker simulates only its *height*. *First seen:
Module 1.*

### overflow / wraparound
When an arithmetic result exceeds the integer type's range. Java `long` wraps
(two's complement, no exception, JLS §15.18.2); JavaScript `BigInt` cannot
overflow. The one input class where the two builds' output legitimately
differs. *First seen: Module 1.*

### production
One rewrite rule: a left-hand nonterminal, `->`, and a right-hand side (a
possibly-empty sequence of symbols). The unit every parsing algorithm selects.
*First seen: Module 4.*

### parse tree (derivation tree)
An ordered tree whose internal nodes are nonterminals, leaves are terminals (or
ε), and each internal node's children are the right-hand side of the production
that expanded it. Records *which* productions and *how they nest* — not the
order applied. *First seen: Module 5.*

### frontier (of a partially-built tree)
The left-to-right sequence of a tree's not-yet-expanded nodes. Building a tree =
repeatedly expanding a frontier node; reading a derivation = repeatedly picking
the leftmost or rightmost expanded nonterminal from a growing frontier. *First
seen: Module 5.*

### pumping lemma
A necessary condition for a language class: **regular** — every long enough
`s ∈ L` splits `xyz` (`|y|≥1`, `|xy|≤p`) with `xyᵏz ∈ L` for all `k`;
**context-free** — five-part `uvwxy` (`|vwx|≤p`, `|vx|≥1`) with `uvᵏwxᵏy ∈ L`.
One-directional: proves *not* regular / *not* context-free by contradiction,
never proves membership. `Pumping.java` runs it as an adversary. *First seen:
Module 7.*

### pumping length (p)
The threshold in a pumping lemma — roughly the number of states in the smallest
automaton. The refutation shows *no* value of `p` works for `{aⁿbⁿ}` (regular)
or `{aⁿbⁿcⁿ}` (context-free). *First seen: Module 7.*

### regular grammar (right-linear / left-linear)
A grammar where every production is `A → w` or `A → wB` (right-linear), or
`A → w` / `A → Bw` (left-linear) — one nonterminal, always at the same end.
Generates exactly the regular languages. `GrammarClass.classify` decides it by
right-hand-side shape. *First seen: Module 7.*

### GrammarClass / Pumping (classes)
`GrammarClass.java` — `classify(g)` → RIGHT_LINEAR / LEFT_LINEAR / CONTEXT_FREE.
`Pumping.java` — `regularRefutation(p)` and `cflRefutation(p)` exhibit, for any
`p`, a string whose every decomposition escapes under pumping. *First seen:
Module 7.*

### panic-mode recovery
On an error, discard input until reaching a *synchronizing token* trusted to
realign the scanner/parser, then resume. `Recovery.java` — `PANIC_ONE` (skip one
char) and `PANIC_TO_SYNC` (skip to a character where a token can start). Parser
version syncs on FOLLOW-set tokens (Module 22). *First seen: Module 17.*

### synchronizing set (sync set)
The set of tokens/characters a recovering scanner or parser is willing to stop
and resume at. Too small → discards too much; too large → resyncs inside the
broken construct and cascades. Real parsers use `;`, `}`, `)`. *First seen:
Module 17.*

### precedence (of operators)
Which operator "binds tighter" and so groups first when parentheses are absent.
In regex: `*` > concatenation > `|` (like exponent > `×` > `+`). Encoded in a
recursive-descent parser as which method calls which — the tightest operator is
the innermost call. In a grammar, as a nonterminal tier per level: the tighter
operator lives deeper, so it nests first. *First seen: Module 3; used to
disambiguate in Module 6; generalised in Module 20.*

### inherently ambiguous language
A context-free language for which *every* grammar is ambiguous, e.g.
`{ aⁱbʲcᵏ : i = j or j = k }`. Disambiguation is impossible, not just hard.
*First seen: Module 6.*

### recognition (vs. parsing)
Deciding whether a string is in a language (yes/no) without building a
structure for it. Parsing additionally produces the structure. Module 1
recognizes; representation starts Module 5. *First seen: Module 1.*

### Regex (class) / regex AST
`Regex.java` / `regex.mjs`: a regular expression parsed into a tree of six node
kinds — `Empty` (∅), `Epsilon` (ε), `Char` (base cases); `Union`, `Concat`,
`Star` (operators). `toLanguage(maxLen)` gives its meaning as a bounded
`Language`; `matches(w)` decides one string exactly via leftover suffixes.
*First seen: Module 3.*

### regular expression (formal)
A tree built from three base cases (`∅`, `ε`, one symbol) and three operators
(union, concatenation, Kleene star). Denotes a regular language. `+`, `?`,
classes, `.` are all sugar over the six; backreferences are not (they leave the
regular languages). *First seen: Module 3.*

### rightmost derivation
A derivation in which every step expands the rightmost nonterminal. Together
with the leftmost derivation, one of the two *canonical* derivations of a parse
tree — each in bijection with the tree. Bottom-up parsers build one in reverse
(Module 27). *First seen: Module 5.*

### ParseTree (class)
`ParseTree.java` / `parsetree.mjs`: `build(g, choices, leftmost)` constructs a
tree from production choices; `derivation(g, tree, leftmost)` reads the leftmost
or rightmost derivation off a finished tree; `terminalYield()` recovers the
string; `productionMultiset()` gives the order-independent bag of rules used.
*First seen: Module 5.*

### yield (of a parse tree)
The string formed by its leaves, left to right. `terminalYield()`. *First seen:
Module 5.*

### linear grammar
A grammar in which every production's right-hand side has at most one
nonterminal. Its leftmost and rightmost derivations coincide for every string,
because no sentential form ever offers a choice of which nonterminal to expand.
*First seen: Module 5.*

### RPN / postfix notation
Reverse Polish Notation: operators follow their operands (`3 4 +` = `3 + 4`).
Chosen for Module 1 because well-formedness needs only a counter, no parser.
*First seen: Module 1.*

### SemanticResult
Java `record` / JS object `(ok, value, message, col)` returned by `evaluate`.
*First seen: Module 1.*

### sentential form
Any string of terminals and nonterminals appearing during a derivation —
`[S]`, `[a, S, b]`, `[a, a, b, b]`. A sentence (member of L(G)) is a sentential
form with no nonterminals. In the code, a `List<String>`. *First seen: Module 4.*

### start symbol / %start
The nonterminal a derivation begins from. Defaults to the first rule's
left-hand side; a `%start X` line overrides it. *First seen: Module 4.*

### static semantics
The meaning-level rules checkable without running the program — types, scope,
declaration-before-use. Contrast **dynamic semantics**. Covered in Modules
42–44. *First seen: Module 1.*

### syntax
The rules of *form*: which strings are well-formed, independent of meaning.
Checked first, by `checkSyntax`. *First seen: Module 1.*

### SyntaxResult
Java `record` / JS object `(ok, message, col)` returned by `checkSyntax`.
*First seen: Module 1.*

### semantics
The rules of *meaning*: what a well-formed string denotes or does. Checked
second, only after syntax passes. *First seen: Module 1.*

### subset construction (powerset construction)
Turning an NFA into an equivalent DFA: a DFA state is a *set* of NFA states.
Start = `εClosure({q0})`; `δ(S, a) = εClosure(⋃ move(s, a))`; `S` accepting iff
`S ∩ F ≠ ∅`. Only reachable subsets are built; worst case `2ⁿ` of them.
`Subset.java` / `subset.mjs`. *First seen: Module 13.*

### Thompson's construction
Compiling a regex tree into an ε-NFA with one small gadget per node
(`Char`/`ε`/`∅` → 2 states; `Concat` → +0; `Union`/`Star` → +2). Every fragment
has one entry and one exit, so gadgets compose freely. Result has ≤ 2·|regex|
states. `Thompson.java` / `thompson.mjs`. *First seen: Module 12.*

### terminal
A grammar symbol with no production — a real token that appears literally in
strings of the language. "Everything in a right-hand side that is never a
left-hand side." Contrast **nonterminal**. *First seen: Module 4.*

### Token
The pair `(text, col)` produced by `tokenize`. *First seen: Module 1.*

### tokenize
Splitting a source line into a list of `Token`s, skipping spaces and tabs and
recording each token's start column. *First seen: Module 1.*

### well-formed
A string that obeys every rule of syntax. Synonym here for "passes
`checkSyntax`." *First seen: Module 1.*
