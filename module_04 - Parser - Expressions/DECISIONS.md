# Module 04 — Design Decisions

## 1. Recursive Descent vs Pratt Parser

**Decision:** Use recursive descent (one function per precedence level).

**Rationale:**  
A Pratt (top-down operator-precedence) parser encodes all precedence levels in a single function using a table of binding-power numbers. It is elegant and very fast to write once you understand the pattern. However, it is notoriously hard to explain to someone seeing parsers for the first time — the binding-power trick feels like magic until you have built several parsers yourself.

Recursive descent, by contrast, maps directly onto the grammar:
- "Higher in the call stack" = "lower precedence"
- Each function is self-contained and easy to read in isolation

At Module 04, clarity beats brevity. A Pratt parser is a good Module 09 exercise.

---

## 2. Left-Recursive Grammar Rewritten as While Loops

**Decision:** Rewrite left-recursive grammar rules as iteration.

**The problem:**  
The "natural" way to express left-associative addition is:

```
additive = additive ('+' | '-') multiplicative   ← left-recursive
         | multiplicative
```

If you turn this directly into a recursive descent function:

```c
static Node *parse_additive(Parser *p) {
    Node *left = parse_additive(p);  // <-- calls itself immediately!
    ...
}
```

`parse_additive` calls itself before consuming any input. The call stack grows without bound until the process crashes with a stack overflow.

**The fix — rewrite as a loop:**

```
additive = multiplicative (('+' | '-') multiplicative)*
```

In code:
```c
Node *left = parse_multiplicative(p);
while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
    advance(p);
    Node *right = parse_multiplicative(p);
    // build a BINARY node, set left = new node
}
return left;
```

The loop consumes at least one token on every iteration (the operator), so it always terminates.

---

## 3. Stubbing Out Statements and Functions

**Decision:** `parse_stmt`, `parse_block`, `parse_function`, and `parse_program` return NULL in Module 04.

**Rationale:**  
Introducing expressions and statements simultaneously would require explaining:
- The AST node types for all constructs at once
- How blocks relate to scoping
- The two-token lookahead needed to distinguish assignment from expression statements

Building one layer at a time lets each module focus on exactly one new concept. The stubs compile cleanly, so the Module 04 binary is a working expression parser that students can experiment with right now.

---

## 4. Storing Operator as a Single `char` vs a `TokenType`

**Decision:** Store the binary/unary operator as a `char` field (`node->op`).

**Rationale:**  
Storing the raw `TokenType` (e.g., `TOK_PLUS`) would be correct but verbose. A single char maps neatly to C's `switch` statement:

```c
switch (n->op) {
case '+': /* emit add */ break;
case '-': /* emit sub */ break;
case 'E': /* emit cmp == */ break;
}
```

The downside is a manual mapping table: `TOK_LEQ` → `'L'`, `TOK_AMPAMP` → `'A'`, etc. This is documented in the NodeKind enum header comment and in this file.

A production compiler (e.g., GCC, Clang) would keep the original token kind and add a separate operator enum. We chose compactness for teaching purposes.

---

## 5. Node Allocation with `malloc` vs an Arena Allocator

**Decision:** Allocate each Node individually with `malloc(sizeof(Node))`.

**Rationale:**  
Individual `malloc` is simple, readable, and requires no infrastructure. Each call to `node_new` is obvious and easy to trace in a debugger.

The cost is fragmentation and O(n) free time (we must walk the entire tree in `node_free`). A production compiler uses an **arena allocator**: a large block of memory is allocated upfront, and nodes are carved out with pointer bumps. Freeing the entire AST is then a single `free(arena)` call — O(1) regardless of tree size.

We defer the arena to Module 07 so students understand the problem before seeing the solution.
