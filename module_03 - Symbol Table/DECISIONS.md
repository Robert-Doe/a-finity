# Module 03 — Design Decisions

## 1. Fixed-size array vs. hash table for symbol storage

**Decision:** Symbols are stored in a plain fixed-size array (`entries[SYMTAB_MAX]`) with linear-scan lookup, not a hash table.

**Why:** At Module 03 the compiler has no hash table implementation, and writing one would be the complexity of a separate module. The programs we compile in this course have fewer than 50 symbols per function, so a linear scan is O(n) over a tiny n — the real-world wall-clock difference between O(n) and O(1) is unmeasurable. Keeping the data structure simple lets students focus on *what* the symbol table records rather than *how* it is indexed.

**Trade-off:** A production compiler (GCC, Clang) uses a hash table so that every identifier lookup — and there are millions in a large translation unit — is O(1). The specific production fix would be to replace `entries[]` with an open-addressing hash table keyed on the identifier's interned string pointer, giving O(1) average-case lookups with no per-lookup `strcmp`.

---

## 2. 64-byte name limit vs. dynamic allocation

**Decision:** `Symbol.name` is a fixed `char name[64]` array embedded directly in the struct, rather than a heap-allocated `char *`.

**Why:** Embedding the string avoids a separate `malloc` per symbol and keeps all symbol data in a single contiguous allocation (the `SymTab` struct itself). Cache locality is better when everything is in one block. 63 usable characters covers every identifier we will write in this course; the C standard only guarantees 31 significant characters in external identifiers.

**Trade-off:** POSIX and C99 allow identifiers of unlimited length (implementations must support at least 63 significant characters for internal identifiers). A name longer than 63 characters will be silently truncated. The production fix is to use an *identifier intern table* (a separate hash map from string → canonical `char *`) and store only the canonical pointer in `Symbol`. This simultaneously removes the length limit and makes equality comparison a pointer compare instead of a `strcmp`.

---

## 3. Single flat table vs. scope stack

**Decision:** There is only one `SymTab` per compilation; it is flat and has no notion of nested scopes.

**Why:** Scope resolution requires understanding which symbols are *visible* at each point in the program, which in turn requires knowing the nesting structure of blocks. That analysis belongs to the semantic analysis phase (Module 07), not the early infrastructure phase. Introducing scope stacks now would force students to understand the problem and the solution at the same time. By building a flat table first, Module 07 can show exactly *why* it breaks and then replace it with a stack of `SymTab` instances — one per scope level.

**Trade-off:** The flat table incorrectly allows symbols with the same name in different functions (the second `symtab_add` call returns -1). The production fix is a *scope stack*: a stack of `SymTab` pointers where `symtab_add` pushes into the top frame and `symtab_lookup` walks down the stack from top to bottom, finding the innermost binding.

---

## 4. No type information in Symbol yet

**Decision:** The `Symbol` struct has no type field. Variables and functions are distinguished only by `SymKind`; there is no `int`, `void`, or pointer type recorded.

**Why:** Designing a type representation is a module in itself. At Module 03 we have not yet defined an AST (Module 04) or a type system (Module 07). Adding a `Type *` field to `Symbol` now would require inventing the entire type hierarchy immediately, which would make Module 03 much harder than it needs to be.

**Trade-off:** Without types, the symbol table cannot support type-checking. The compiler will accept `int x; x = add;` without complaint. The production fix is to add a `struct Type *type` pointer to `Symbol` once the `Type` representation is defined in Module 07, and to populate it during a separate type-annotation pass.

---

## 5. Linear search vs. binary search

**Decision:** `symtab_lookup` performs a linear scan through `entries[0..count-1]`, comparing names with `strcmp`.

**Why:** Binary search requires the entries to be sorted, which means either sorting on every insert (O(n log n) insertions) or re-sorting before every lookup (O(n log n) per lookup unless deferred). Adding that complexity for a table of fewer than 50 entries provides no practical benefit and makes the code harder to read.

**Trade-off:** At scale (thousands of symbols per scope), O(n) lookup is the dominant compile-time cost. The production fix is, again, the hash table from Decision 1 — which makes binary search moot. If a sorted array were preferred (e.g., for memory-constrained embedded toolchains), insertion sort on each `symtab_add` plus `bsearch` on lookup would give O(log n) with no extra memory overhead beyond the array itself.
