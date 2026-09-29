# Module 16 — Decisions

Choices in `Scanner.java` / `scanner.mjs` and `Pattern.java` / `pattern.mjs`,
sorted into **(a) forced** by the algorithm, **(b) fixed by the language being
scanned** (Ajoda), and **(c) our own convention**.

---

## The spec file

### Three directives: `token`, `discard`, `keywords` — (c)
A spec line says what to *do* with a match, not just what it's called.
`token` emits, `discard` matches and drops (whitespace, comments), and
`keywords` names the rule whose lexemes get a table lookup afterwards. Reading
a spec top to bottom tells you everything the scanner will do with no
side-channel flags.

### Errors name the spec line — (c)
`spec line 3: SIGN accepts the empty string; ...` A spec is source code, so its
errors point at a line like any other compiler error.

### `#` in column 1 is a comment; `#` elsewhere is pattern text — (c)
Patterns may need a literal `#`, so only a whole-line comment is recognized.

---

## The pattern language

### Classes, ranges, negation, escapes, quoted literals — (b)
Ajoda's lexical grammar uses `[A-Za-z_]`, `[^\n]` and multi-character
operators like `->`. Module 3's syntax can't express those without escaping,
so this layer adds exactly those features and nothing else.

### Everything desugars to Module 3's six node kinds — (a)
`[0-9]` becomes `0|1|…|9`; `"->"` becomes `-` then `>`. Thompson (Module 12)
and the subset construction (Module 13) run unmodified, which keeps each
earlier module's guarantees intact.

### Four characters get symbol names: SP, TAB, NL, HASH — (a)
The automaton text format (Module 10) splits on whitespace and uses `#` for
comments. Renaming those four characters is the smallest change that lets the
existing loaders carry them.

### Bare whitespace in a pattern is an error — (c)
`[\s\t\n]+` and `" "` are unambiguous; a stray space usually means a typo that
split one pattern into two.

### Source alphabet = tab, newline, printable ASCII — (c)
Negated classes need a finite universe to subtract from. This one is 97
symbols, covers Ajoda, and keeps the DFA table small. Anything else is a scan
error reported as `U+XXXX`.

---

## Building the automaton

### One NFA per rule, joined under a fresh START with tagged accepts — (a)
The standard construction for combining token patterns. Each rule's accept
state maps to its rule index in `acceptRule`.

### A DFA state's rule = the smallest rule index it accepts — (c)
Ties between equally long matches go to the earliest-declared rule. This is
the tie-break `lex`/`flex` use and the one Module 15 motivates.

### Record the dead state (`sink`) — (a)
The subset construction produces a state for the empty set that loops to
itself. Stopping the scan there keeps scanning linear.

### Refuse any pattern whose NFA accepts `""` — (a)
`εClosure({start}) ∩ accept ≠ ∅` means the rule can match nothing. Checked
per rule at load time, so the error names the rule and its line.

---

## Scanning

### Longest match, then earliest rule — (a)
Scan until the DFA can't continue, then back up to the last accept.

### Keywords by lookup after the match — (b)
Ajoda's lexer spec says reserved words are matched as `IDENT` and then looked
up, so `iffy`, `while1` and `truest` stay identifiers. The keyword's kind is
the word in upper case (`if` → `IF`, `i64` → `I64`).

### `line:col`, 1-based, at the start of each token — (b)
Ajoda requires every diagnostic to cite a location, and the lexer is the last
stage that sees raw characters. Discarded lexemes still advance the position.

### End with an `EOF` token — (b)
The parser consumes a stream that ends in `EOF`, so it never needs a
bounds check.

### A scan error keeps the tokens found so far (`partial`) — (c)
The driver shows how far the scan got before the bad character. Module 17
builds real recovery on top of this.

---

## Scorecard

| # | Decision | Kind | Rejected alternative |
|---|----------|------|----------------------|
| 1 | `token` / `discard` / `keywords` directives | (c) | a flag syntax bolted onto `NAME regex` lines |
| 2 | spec errors cite the line | (c) | errors that name only the rule |
| 3 | classes, escapes, quoted literals | (b) | Module 3's bare syntax, which can't spell Ajoda |
| 4 | desugar to six node kinds | (a) | a second regex engine |
| 5 | SP / TAB / NL / HASH symbol names | (a) | rewriting the automaton file format |
| 6 | 97-symbol ASCII alphabet | (c) | full Unicode (huge classes, huge DFA) |
| 7 | earliest rule wins ties | (c) | longest pattern text wins (fragile) |
| 8 | stop at the dead state | (a) | walk to end of input on every token (O(n²)) |
| 9 | refuse empty-matching patterns at load | (a) | silently never firing that rule |
| 10 | keywords by lookup | (b) | one rule per keyword (bigger DFA) |
| 11 | 1-based `line:col` | (b) | a flat character offset |
| 12 | trailing `EOF` | (b) | parser checks for end of list |
