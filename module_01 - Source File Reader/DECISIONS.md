# DECISIONS.md — Module 01: Source File Reader

Design decisions made in this module, with reasoning and production trade-offs.

---

## 1. Read the entire file into memory at once (not streamed line-by-line)

**Decision:** `source_open()` reads all bytes in a single `fread()` call into a heap buffer.

**Why:** The lexer (Module 02) must look one character ahead of its current position to distinguish `=` (assignment) from `==` (equality). Streaming one byte at a time would force us to buffer that lookahead anyway — we'd re-implement a partial buffer and end up with more code and the same memory footprint. A single flat buffer also gives every later module random access to any byte by index, which is essential for reporting precise error locations ("line 5, column 12 of main.c"). Keeping it simple in Module 01 lets us focus on the read mechanics without inventing an I/O abstraction layer.

**Trade-off:** A 500 MB source file (generated code, machine-produced tables) will exhaust RAM on a small system. The production fix is `mmap()` on Linux / `MapViewOfFile()` on Windows — the OS memory-maps the file, giving the same random-access semantics with zero up-front copy and demand-paged loading. We deliberately avoid this in Module 01 because it introduces platform-specific headers (`<sys/mman.h>`) before we have a cross-platform build structure in place.

---

## 2. Measure file size with `fseek`/`ftell`, not `stat()`

**Decision:** We seek to the end of the open file and call `ftell()` to obtain the byte count.

**Why:** `stat()` is a POSIX function, not part of the C11 standard. Using it would require `#include <sys/stat.h>`, a platform dependency we have not formally introduced yet. `fseek`/`ftell` is guaranteed by the C standard and compiles without modification on Linux, WSL, and macOS. At Module 01, portability costs nothing and the performance difference is negligible for the file sizes we target.

**Trade-off:** `fseek(fp, 0, SEEK_END)` on a binary stream is technically implementation-defined under C11 §7.21.9.2 — the standard says the result is unspecified for files opened in binary mode. In practice every major OS implements it correctly, but a strict reading of the standard says we are relying on undefined behaviour. The production fix is `fstat(fileno(fp), &st)` followed by `st.st_size` — always defined on POSIX, correct for files and block devices, immune to race conditions between the seek and the read.

---

## 3. Call `exit(1)` on error rather than returning a sentinel

**Decision:** `source_open()` calls `exit(1)` on fatal errors instead of returning an error code or null pointer.

**Why:** At Module 01, there is no error-handling infrastructure — no result type, no logging system, no recovery strategy. Forcing the caller to check a return code every time would require inventing three supporting systems (an error enum, a result wrapper, a message formatter) before we can read a single file. Calling `exit()` keeps the code readable and the module focused on its single responsibility: loading bytes into memory.

**Trade-off:** A library must never call `exit()` — it makes the module untestable (you cannot catch `exit()` in a unit test framework without forking a subprocess) and unusable in any long-running process such as a language server or batch compiler. The production fix is to return a `bool` success flag and fill an out-parameter error struct, or to use a `longjmp`-based error context that the driver installs before calling any compiler function.

---

## 4. Store `filename` as a borrowed pointer, not a `strdup()` copy

**Decision:** `Source.filename` points directly at the path string the caller passed in; we do not copy it.

**Why:** At Module 01, `source_open()` is called exactly once from `main()` with `argv[1]`, which lives for the entire program lifetime. Copying the string would require an extra `malloc`, an extra `free` in `source_free`, and a new error path for the copy — three additions with zero benefit for our current use case. Borrowing the pointer keeps the struct small and the ownership model simple.

**Trade-off:** If a caller ever frees or mutates the path string while holding a live `Source`, `src.filename` becomes a dangling pointer that will produce corrupt error messages or a segfault. This is not a risk today because `argv[1]` is immutable and long-lived, but it will become a risk in Module 16 when we process multiple input files in a loop. The fix is to `strdup()` the filename in `source_open()`, `free(src->filename)` in `source_free()`, and document that `Source` fully owns all its fields.

---

## 5. Use `size_t` for lengths, not `int`

**Decision:** `Source.len` and all loop counters over the source buffer are `size_t`, not `int` or `long`.

**Why:** `int` is 32 bits on all common 64-bit platforms. A source file larger than 2 GB (generated C, embedded data tables) would overflow a signed 32-bit length, causing the read-size check to pass incorrectly and the buffer to be under-allocated. `size_t` is the type that `malloc()` and `fread()` use for sizes — matching their types eliminates signed/unsigned comparison warnings from `-Wextra` and is semantically correct.

**Trade-off:** `size_t` is unsigned, so `(size_t)0 - 1` silently wraps to `SIZE_MAX` rather than erroring. In Module 02, the lexer will compute expressions like `src->len - current_pos`. If `current_pos` is ever greater than `src->len` due to a bug, the subtraction wraps and the lexer reads far past the end of the buffer. We will add an explicit bounds-check before every such subtraction starting in Module 02.

---

## 6. Return `Source` by value, not by heap pointer

**Decision:** `source_open()` returns a `Source` struct directly on the stack, not a `Source*` pointing to a heap allocation.

**Why:** On a 64-bit system, `Source` is 24 bytes (one 8-byte pointer + one 8-byte `size_t` + one 8-byte pointer). Returning it by value copies 24 bytes — cheaper than allocating a separate heap object for the wrapper struct and dealing with the extra ownership question of who frees the wrapper versus who frees `src->text`. Returning by value also makes the caller's code cleaner: `Source src = source_open(path)` rather than `Source *src = source_open(path); ... free(src);`.

**Trade-off:** If `Source` grows to carry auxiliary data — a line-offset table for O(1) line-number lookup, a list of macro expansion records, an interned string table — returning it by value becomes expensive. The fix at that point is to heap-allocate `Source` in `source_open()` and return a `Source*`, with `source_free()` freeing the struct itself in addition to `text`. We will make this change in Module 07 when the struct first gains significant auxiliary data.
