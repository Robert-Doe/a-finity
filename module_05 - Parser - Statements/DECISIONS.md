# Module 05 Design Decisions

## 1. Recursive Descent vs Table-Driven Parsing

We chose **recursive descent** over a table-driven approach (like LALR(1) used by Yacc/Bison).

**Why recursive descent?**
- Each grammar rule maps directly to a C function with the same name.  When you read `parse_stmt`, you can immediately match it to the grammar rule for `stmt`.  A table-driven parser hides the grammar behind rows and columns of numbers.
- Error messages are trivial: you are already inside the function that knows what it expected, so you can say "expected ';' after return expression" rather than "syntax error at state 47".
- Adding a new statement kind takes five minutes: write the function, add a branch to `parse_stmt`.
- The trade-off is that recursive descent cannot handle left-recursive grammars directly.  We solve that with while-loops (see Decision 3).

---

## 2. Two-Token Lookahead (cur + peek)

The Parser struct holds two pre-fetched tokens: `cur` (what we are looking at) and `peek` (one step ahead).

**Why two tokens?**
The critical ambiguity is assignment vs expression:
- `x = 5;`  — assignment statement
- `x + 1;`  — expression statement starting with identifier `x`

Both start with `TOK_IDENT`.  With only one token of lookahead we would have to consume the identifier before knowing which path to take, complicating backtracking.  With `peek` we can check `cur == IDENT && peek == '='` without consuming anything, then commit to the assignment parse.

A single-token lookahead would also force complex "un-get" logic or require restructuring the grammar.  Two tokens is the simplest solution that keeps the grammar natural.

---

## 3. Eliminating Left Recursion with While Loops

A left-recursive rule like:

    additive = additive '+' multiplicative | multiplicative

would make `parse_additive` call itself immediately — infinite recursion and a stack overflow.

**Our fix:** convert to an iterative (while-loop) rule:

    additive = multiplicative ('+' multiplicative)*

The while loop accumulates binary nodes left-to-right, which produces the same left-associative tree without recursion.  Every binary operator level (logical, comparison, additive, multiplicative) uses this pattern.

---

## 4. Error Recovery Strategy: exit(1) on First Error

When `expect()` or `parse_error()` detects a syntax problem, we print a message and call `exit(1)` immediately.

**Why not try to recover?**
- Panic-mode recovery (skip tokens until a synchronisation point like `;` or `}`) surfaces multiple errors per run but adds 200+ lines of tricky state management.
- For a teaching compiler the first error is usually the most informative.  Cascading errors from a bad parse state produce confusing noise.
- Students learning C compilers benefit more from seeing one clear message than from a list of confused follow-on errors.

A future module could add recovery by catching errors with `setjmp/longjmp` and skipping to the next `;` or `}`.

---

## 5. AST_BLOCK Uses an args[] Array (Not a Linked List)

A block's statements are stored in `node->args[]`, a heap-allocated array grown with `realloc`.

**Why an array?**
- Later passes (semantic analysis, IR generation, code generation) iterate over statements sequentially by index.  `args[i]` is O(1); linked-list traversal is O(n) and requires following pointers on each step.
- `realloc` keeps the elements contiguous in memory, which is cache-friendly.
- The AST_CALL and AST_FUNC nodes already use `args[]` for arguments and parameters, so blocks re-using the same field keeps the Node struct uniform — one shape fits all node kinds.

The cost is that we do not know the statement count in advance and must realloc one slot at a time.  For the sizes of programs we parse in this course that is not a bottleneck.

---

## 6. void Parameter Lists

C lets a programmer write `int main(void)` to explicitly declare that a function takes no arguments (as opposed to `int main()` which in C means "unspecified parameters").

**How we handle it:**
Inside `parse_func`, after consuming `(`, we check whether the next token is `void`.  If it is, we consume it and produce an AST_FUNC with `nargs == 0` — indistinguishable from a function with an empty parameter list.  No special AST node or flag is needed.

The alternative — using a null parameter node or a dedicated AST_VOID_PARAMS node — would complicate every later pass that inspects parameter lists without adding any useful information (zero is zero).
