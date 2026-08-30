# Module 01 — Decisions

Every non-obvious choice in `java/Rpn.java`, `java/Main.java`, and their
JavaScript twins, sorted into three buckets:

- **(a) forced** — the platform, the language, or the definition of RPN left no
  choice. Not a decision, just a fact we wrote down.
- **(b) external contract** — forced by something outside this module: a library
  function's behavior, the course-wide "both builds emit identical bytes" rule,
  the fixture file format.
- **(c) our convention** — a real choice, made for clarity or safety, where a
  different consistent choice would also have worked.

---

## The language itself

### RPN (postfix), not infix — (c)
`3 4 +`, not `3 + 4`. Postfix can be checked for well-formedness with a single
integer counter and **no parsing at all** — exactly the machinery this module
wants to avoid, because parsing technique is Modules 15+. Infix would drag a
precedence parser into lesson one and bury the point.

### Integer literals are unsigned `[0-9]+`; `-` is only the operator — (c)
`-5` is **not** a token. A negative *value* only ever arises from evaluating
`0 5 -`. This sidesteps the classic lexical ambiguity — is `-5` a negative
literal or the operator `-` applied to `5`? — which real lexers resolve with
maximal munch plus context (Module 15). Lesson one does not need that fight.

### An expression must leave exactly one value — (a)
This is what "an RPN expression" *means*: it denotes one number. Final stack
depth 0 ⇒ nothing was computed; depth > 1 ⇒ operands with no operator to combine
them. The `depth != 1` check in `checkSyntax` is the definition, not a policy.

---

## Numbers and arithmetic

### Java `long`, JavaScript `BigInt` — (a), language-forced
Java's `long /` does integer division. JavaScript's `Number /` does **not**:
`7 / 2 === 3.5`, and `5 / 0 === Infinity` with no error. To get identical
behavior to Java — truncate toward zero, and a *real* signal on divide-by-zero —
the JS build must use `BigInt`. `7n / 2n === 3n`; `5n / 0n` throws. This is the
first genuine "the two languages force different code" divergence in the course,
and there will be a `<figure>` for it in every tutorial from here on.

### Division truncates toward zero — (a), a consequence of the types above
`0 5 - 2 /` is `(-5) / 2` and the answer is **-2, not -3**. Both `long` and
`BigInt` truncate toward zero. We did not choose this rounding; we chose the
types, and this came with them. The test `eval("0 5 - 2 /") == -2` pins it so a
future refactor to floor division would fail loudly.

### Divide-by-zero is checked with `if (b == 0)` *before* dividing — (b)
If we let the division throw, Java raises `ArithmeticException("/ by zero")` and
JS raises `RangeError("Division by zero")` — two types, two messages, neither
carrying a source column. Checking first lets both builds emit the **same**
diagnostic: `col 6: division by zero`. Uniform diagnostics are a course
convention; the explicit guard is how we keep it.

### `Long.parseLong(t)` / `BigInt(t)` are only called after `isInteger(t)` — (b)
Both throw on malformed input. We only reach them inside the `isInteger(t)`
branch, where every character is already known to be a digit. The guard is the
contract; the parse call trusts it.

---

## The two checkers

### `checkSyntax` simulates stack **depth**, never a value — (c)
It counts `+1` per integer, `-1` per operator, and never calls `parseLong`. This
is the module's thesis written as code: *well-formedness is decidable without
running the program.* A perfectly consistent alternative is to fold both checks
into `evaluate` and report both error kinds from one pass — it would produce the
same verdicts. We split them precisely because the split **is** the lesson.

### `checkSyntax` reports only the first error — (c)
Reporting several errors from one pass needs error recovery (deciding where to
resume), which is Module 22. One error keeps `SyntaxResult` a plain
`(ok, message, col)` triple.

### Empty input fails with `"empty expression"` and `col 0` — (c)
Zero values on the stack is ill-formed (see "must leave exactly one value").
`col 0` is our sentinel for "this error is about the whole input, not a
position" — `Main`'s `at()` helper prints the message alone when `col == 0`.

### Results are returned as data, never thrown — (c)
`SyntaxResult` / `SemanticResult` are Java `record`s; the JS twins are plain
`{ok, message, col}` objects with the same field names. A failed check is an
*expected* outcome, so it is a return value, not an exception. Both checkers stay
pure functions: same input → same object.

### Columns are 1-based — (c)
Every compiler and editor the learner has used reports 1-based columns. Matching
that costs exactly one `+ 1`, in `tokenize` (`col: start + 1`). Being the odd
one out with 0-based columns would be a papercut in every later module.

---

## Plumbing

### Output is assembled in a buffer and printed once, with explicit `\n` — (a)
Java's `System.out.println` appends `System.lineSeparator()` — `\r\n` on Windows.
Node's `console.log` appends `\n`. The course promises the Java and JS builds
emit **byte-identical** output, so `Main` builds a `StringBuilder` with `"\n"`
and ends with a single `System.out.print`. `RpnTest` still uses `println` — its
output is not a golden file.

### Fixture lines are wrapped in double quotes — (b)
`"3 4 +"`, and `""` for the empty expression. The quotes let one plain text file
carry the empty string, and any leading/trailing spaces, with no ambiguity on
any platform. `#` comments and blank-line skipping are ordinary (c) convenience.

### `tokenize` splits on space and tab only — (c)
Not `\n`, `\r`, `\f`, `\v` — the input is a single line, so they cannot occur.
General whitespace handling is Module 13. Keeping the set tiny here keeps
`tokenize` to four lines you can hold in your head.

---

## Decisions We Made

| # | Decision | Bucket | A different choice would have… |
|---|----------|--------|-------------------------------|
| 1 | Postfix (RPN), not infix | (c) | …needed a precedence parser in lesson 1 |
| 2 | Unsigned integer literals; `-` is only the operator | (c) | …forced a maximal-munch lexical rule now instead of Module 15 |
| 3 | Java `long` / JS `BigInt` | (a) | …made JS `/` return `3.5` and `Infinity`, breaking parity with Java |
| 4 | Truncate toward zero | (a) | …(floor division) needed a custom `Math.floorDiv`-style helper in both |
| 5 | Guard `if (b == 0)` before dividing | (b) | …surfaced two different exception types with no column |
| 6 | `checkSyntax` counts depth, never values | (c) | …(one combined pass) worked, but hidden the syntax/semantics line |
| 7 | First error only | (c) | …needed Module 22's recovery to decide where to resume |
| 8 | Empty input ⇒ syntax error, `col 0` | (c) | …(valid, value "none") contradicted "an expression denotes one value" |
| 9 | Results as returned data, not exceptions | (c) | …made the checkers impure and harder to test |
| 10 | 1-based columns | (c) | …disagreed with every compiler the learner has seen |
| 11 | Buffer + single `print` with `\n` | (a) | …left Windows output as `\r\n`, breaking byte-parity |
| 12 | Quoted fixture lines | (b) | …couldn't represent the empty expression unambiguously |

---

## What We Proved

1. **A string can be syntactically valid and semantically invalid.**
   `12 0 /` — `checkSyntax` returns `ok`, `evaluate` returns
   `division by zero`. Verified by `RpnTest` / `rpn.test.mjs` and visible as
   input `[4]` in `expected/main.out`.

2. **Well-formedness is decidable without executing the program.**
   `checkSyntax` never parses an integer and never does arithmetic, yet it
   correctly accepts every runnable expression and rejects every un-runnable
   one. The proof that it *only* checks form: it also accepts `12 0 /`, which
   does not run.

3. **The two checkers disagree on exactly one of the seven fixtures**, and that
   one (`syntaxOkButSemanticFail=1` in the summary) is the whole reason a
   compiler has separate front-end passes.

4. **Two languages, one behavior.** The Java and JavaScript builds produce
   byte-identical output (`diff` in `run.md` step 4), which required three
   deliberate divergences in the *code* (`BigInt`, the divide-by-zero guard, the
   output buffer) to achieve.
