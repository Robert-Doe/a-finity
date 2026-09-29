# Module 06 — Design Decisions

## 1. Storing return type as `char` in `node->op` ('i'/'v')

**Decision:** The return type of a function is encoded as a single character stored in the `op` field of the `AST_FUNC` node: `'i'` for `int`, `'v'` for `void`.

**Rationale:** At this stage of the compiler we have no type system yet, so introducing a full `TypeKind` enum would be premature. A single character is enough to distinguish the two types our language supports, and it reuses an existing field in the fat-node struct without adding new fields.

**Production fix:** A real compiler would define a `TypeKind` enum (or a richer `Type *` pointer to a type table), and store that in a dedicated `type` field. This enables handling pointer types, struct types, arrays, and generic type annotations.

---

## 2. Parameters represented as `AST_VAR_DECL` nodes (without initialiser)

**Decision:** Each formal parameter in a function definition is stored as an `AST_VAR_DECL` node with `left = NULL` (no initialiser).

**Rationale:** Reusing the same node kind for parameters and local variable declarations keeps `node_print`, `node_free`, and the upcoming semantic analysis pass simpler — they only need to handle one kind of declaration node. The downside is that parameters and locals look identical in the tree unless the caller checks context (are we inside `args[]` of an `AST_FUNC`, or inside `args[]` of an `AST_BLOCK`?).

**Production fix:** Introduce a dedicated `AST_PARAM` node kind, or add a flag field to `AST_VAR_DECL` that distinguishes parameters from locals. This makes later passes more robust.

---

## 3. `parse_program` loops until non-function token (silent stop)

**Decision:** `parse_program` loops as long as the current token is `TOK_KW_INT` or `TOK_KW_VOID`. Any other token at the top level stops the loop without an error message (beyond the warning printed for non-EOF).

**Rationale:** Keeping the loop condition simple avoids false positives during development when the sample file might have trailing whitespace or an unrecognised construct. The warning line gives enough signal during debugging.

**Production fix:** After the loop, if the current token is not `TOK_EOF`, emit a hard error: `"error: unexpected top-level token"`. This prevents silent partial parses.

---

## 4. No function prototypes / forward declarations

**Decision:** Functions must be fully defined before they are called. There is no `int foo(int x);` prototype syntax.

**Rationale:** This eliminates the need for a two-pass symbol-table build in the parser. The parser doesn't track names at all — it only builds the AST. The semantic analysis pass in Module 07 is where forward references are handled (by doing a first pass over `AST_PROGRAM` to collect function signatures before checking call sites).

**Production fix:** Support declarations as well as definitions, and enforce the rule that a call site must have seen at least a declaration. This is the C header model.

---

## 5. `void` in parameter list treated as zero-parameter marker

**Decision:** If the parameter list is exactly `(void)` — i.e., the current token is `TOK_KW_VOID` and the next is `TOK_RPAREN` — we consume the `void` and treat the function as having zero parameters.

**Rationale:** This matches the C99/C11 distinction between `f()` (unspecified parameters in C89; zero parameters in C++) and `f(void)` (explicitly zero parameters). Our language follows the C99 convention where `f(void)` is the idiomatic way to declare a no-argument function.

**Edge case:** `void f(void x)` — a parameter named `x` of type `void` — would be mis-parsed as a zero-parameter function followed by a stray identifier. A production compiler would catch this in semantic analysis (void as a parameter type is illegal except as the lone `void` marker).
