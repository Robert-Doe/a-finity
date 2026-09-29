# Module 16 — Design Decisions

## 1. Why we shell out to nasm + gcc instead of doing everything in-process

We call `nasm` and `gcc` (as a linker front-end) via `system()` rather than
emitting raw ELF bytes ourselves. The reasons are pedagogical and practical:

- **Complexity budget.** Writing a correct ELF64 emitter with proper section
  alignment, relocation records, and symbol tables would be another 1,000+ lines
  of fiddly bit-manipulation — roughly as much code as everything we have already
  written. That complexity belongs in a dedicated "linker/object-file" course,
  not here.
- **Correctness guarantee.** NASM is a battle-tested assembler. Using it means
  our assembly text is authoritative: if `mycc` produces correct NASM, we get
  a correct binary. Bugs in a hand-rolled ELF writer would be nearly impossible
  to diagnose without an external reference.
- **ABI and OS details are handled for us.** `gcc -no-pie` takes care of the
  C runtime start-up (`_start` → `main`), stack alignment, and the dynamic
  linker stub. Replicating all of that is a distraction.
- **Real compilers do this too.** GCC itself shells out to `as` and `ld`. LLVM
  shells out to system linkers. The two-step compile-then-link model is the
  industry norm, not a shortcut.

## 2. How the test harness validates correctness

`run_tests.sh` uses **exit code** as the primary correctness signal and
**stdout** as a secondary signal for programs that call `print`:

- Exit code: the process exit status is observable without any runtime support.
  It doubles as a compact "return value" — `return 55;` in `main` makes the
  shell see exit 55. This is exactly how `gcc`'s own test suite works (DejaGNU
  checks `exit`).
- Stdout: captured with `$(/tmp/test_bin)`. Used only when a test uses `print`,
  so that we can verify `print_int` actually writes the right digits.
- Why not compare full assembly output? Assembly is fragile across optimisation
  levels and register allocators. Behaviour testing (exit code + stdout) is the
  only portable, future-proof approach.

## 3. Why timing statistics matter for students

The per-phase timing table printed by the driver builds **intuition** about
where compiler time actually goes:

- For a small program (< 100 tokens), parsing typically takes 10–50× longer
  than lexing, because the parser does more work per token.
- Semantic analysis and IR generation are usually faster than parsing for the
  same input.
- Code generation surprises students: it is often the fastest phase of all,
  because we are doing one linear pass over the IR with no search.
- The `nasm` and `gcc` calls dominate the total wall time — sometimes by 100×.
  This teaches students why incremental compilation (avoiding re-running the
  assembler for unchanged files) matters so much in production build systems.

## 4. What this compiler cannot do that a real C compiler can

This is an intentional teaching subset. Missing features include:

- **Types beyond `int`:** no `char`, `float`, `double`, `long`, `unsigned`, etc.
- **Aggregate types:** no structs, unions, or arrays. All variables are scalars.
- **Pointers and addresses:** no `&`, `*`, pointer arithmetic, or dynamic memory.
- **Strings:** no string literals (`"hello"`), no `printf`, no `puts`.
- **The C preprocessor:** no `#include`, `#define`, `#ifdef`, or macros of
  any kind. Each source file must be self-contained.
- **Multiple source files / separate compilation:** `mycc` compiles exactly one
  `.c` file per invocation. There is no object-file cache or linker script.
- **Standard library:** the only runtime function is `print_int` from
  `runtime.asm`. There is no `malloc`, `free`, `fopen`, or any POSIX API.
- **Floating point:** the FPU/SSE registers are not used.
- **Variadic functions:** no `...` parameter lists.
- **Function pointers and higher-order functions:** functions are not first-class
  values.
- **`goto`, `switch`, `break`, `continue`:** only `if`/`while` control flow.
- **Full type system:** no implicit conversions, no type-checking beyond
  "variable declared or not".

Each omission was deliberate: adding any one of them would roughly double the
code size and shift the focus from pipeline architecture to feature minutiae.

## 5. Suggested extensions for students who want to go further

1. **Add arrays:** introduce `AST_INDEX`, lower array accesses to address
   arithmetic in the IR (`IR_ARRAYLOAD`, `IR_ARRAYSTORE`), and allocate
   contiguous stack space in the code generator. The interesting challenge is
   bounds-checking.

2. **Add strings:** put string literals in a `.rodata` section, represent them
   as `const char *` (a pointer IR operand), and wire up `print_str` in
   `runtime.asm` using the `sys_write` syscall.

3. **Multiple source files:** add a simple `#include`-like directive that reads
   and parses a second file, merges its AST into the current program node, and
   emits all functions into one `.asm` file. This teaches you what a linker
   resolves.

4. **Real register allocator:** replace the stack-slot-per-temp strategy with
   Chaitin-Briggs graph coloring. Build a liveness analysis pass, construct an
   interference graph, color it with the 15 general-purpose x86-64 registers,
   and spill only what cannot be colored.

5. **ARM64 backend:** add a second `codegen_arm64.c` that emits AArch64
   assembly instead of x86-64. The IR is already architecture-neutral, so only
   the backend changes. Test on a Raspberry Pi or an Apple M-series Mac under
   Rosetta/native.

6. **Self-hosting:** rewrite `mycc` itself in the subset of C that `mycc` can
   already compile — no structs, no standard library calls other than `print`.
   Getting a compiler to compile itself (even a tiny version) is one of the most
   satisfying milestones in programming language implementation.
