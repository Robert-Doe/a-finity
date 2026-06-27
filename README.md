# A-finity

**A C compiler built from source-file bytes to a linkable x86-64 ELF object, one mechanism at a time — because you cannot reason about miscompilation, ABI confusion, or code-generation bugs in software you have only ever consumed through `gcc -O2`.**

## What this is

A-finity is a hand-written, from-scratch compiler for a teaching subset of C, taken all the way from "read bytes off disk" to "produce a linkable x86-64 object file that runs." It is the first half of a two-part systems series (the second half, `theBud OS`, boots and runs the binaries this compiler emits).

The point is not to reproduce GCC. The point is to build every stage a real compiler front-and-back-end has to have — lexer, symbol table, recursive-descent parser, semantic analysis, an IR, optimization passes, register allocation, x86-64 codegen, and an ELF emitter — by hand, in C, so that every design decision (and every corner cut to keep the module tractable) is visible and documented. That is also the AppSec-education angle: most memory-corruption and logic-confusion bugs in real compilers and interpreters live in exactly these stages (lexical edge cases, symbol resolution, calling-convention mismatches, unchecked buffer math in code emission). You cannot reason like a vulnerability researcher about a code-generation bug if you have never generated code.

Every module ships a `DECISIONS.md` that records not just what was built, but *why* — including the production trade-off being deliberately deferred (e.g. "linear-scan symbol table now, hash table when Module 07 needs it"; "deep-copy semantics now, arena allocation once lifetimes matter"). Reading the `DECISIONS.md` files in order is effectively a running log of a compiler-construction course.

A companion deep-dive, `everything_parsing/`, rebuilds the parsing theory this compiler depends on — regular languages, DFA/NFA construction, LL(1) table-driven parsing, and shift-reduce/LR(0) items — with a working Java implementation and a working JavaScript implementation side by side for every concept, modeled on the rigor of a formal PL-principles course (ASU CSE 340) and the Dragon Book.

## Module map

| # | Module | What it builds |
|---|--------|-----------------|
| 01 | [Source File Reader](./module_01%20-%20Source%20File%20Reader) | Load a source file into a single flat buffer; the foundation every later stage indexes into |
| 02 | [Lexer / Tokeniser](./module_02%20-%20Lexer%20-%20Tokeniser) | Hand-written scanner: keywords, identifiers, operators, `TOK_ERROR` recovery instead of `exit()` |
| 03 | [Symbol Table](./module_03%20-%20Symbol%20Table) | Flat linear-scan symbol storage; the deliberate limitation Module 07 will break and fix |
| 04 | [Parser — Expressions](./module_04%20-%20Parser%20-%20Expressions) | Recursive-descent precedence climbing; left-recursion rewritten as iteration |
| 05 | [Parser — Statements](./module_05%20-%20Parser%20-%20Statements) | Two-token lookahead to disambiguate assignment vs. expression statements |
| 06 | [Parser — Functions](./module_06%20-%20Parser%20-%20Functions) | Function definitions, parameters, and the AST shapes that carry them |
| 07 | [Semantic Analysis](./module_07%20-%20Semantic%20Analysis) | Two-pass checking (collect signatures, then validate bodies) so mutual recursion works |
| 08 | [IR Generation](./module_08%20-%20IR%20Generation) | Lowering the AST to three-address code (TAC) |
| 09 | [IR Optimization](./module_09%20-%20IR%20Optimization) | Local constant folding, copy propagation, dead-code elimination as separable passes |
| 10 | [x86-64 Code Generation](./module_10%20-%20x86-64%20Code%20Generation) | Naive stack-slot codegen: every temporary gets a fixed `[rbp - N]` slot |
| 11 | [Code Generation — Control Flow](./module_11%20-%20Code%20Generation%20-%20Control%20Flow) | Branches, loops, and idiomatic `test`/`jz` control flow |
| 12 | [Function Calls & the System V AMD64 Calling Convention](./module_12%20-%20Function%20Calls%20%26%20the%20System%20V%20AMD64%20Calling%20Convention) | Stack alignment, argument register spilling, ABI-correct prologues |
| 13 | [Register Allocator (Graph Coloring)](./module_13%20-%20Register%20Allocator%20\(Graph%20Coloring\)) | Live-range analysis and graph-coloring allocation into callee-saved registers |
| 14 | [ELF Object File Emitter](./module_14%20-%20ELF%20Object%20File%20Emitter) | Hand-rolled `ET_REL` ELF64 writer — sections, symtab, strtab, relocations |
| 15 | [Standard Library Shim](./module_15%20-%20Standard%20Library%20Shim) | A `print` built-in and hand-written integer-to-string conversion, no libc dependency |
| 16 | [The Complete Compiler](./module_16%20-%20The%20Complete%20Compiler) | End-to-end driver (`nasm` + `gcc` as linker) with a real test suite |
| — | [everything_parsing/](./everything_parsing) | Standalone parsing-theory deep dive: regex → DFA/NFA → Thompson/subset construction → LL(1) and LR(0) parsing, in parallel Java and JavaScript implementations |

## Tech stack

- **C11** — the compiler itself (lexer, parser, semantic analysis, IR, codegen, ELF emitter)
- **x86-64 assembly (NASM syntax)** — the runtime shim and the code the compiler emits
- **GNU Make** — per-module build files
- **Java + JavaScript (Node/`.mjs`)** — dual reference implementations for the `everything_parsing` theory track
- Target: **x86-64 ELF (`ET_REL`)**, linked with `gcc -no-pie` as the linker front end

## Status

Work in progress. Modules 01–16 each contain a working, buildable stage of the pipeline (`Makefile`, source, `tutorial.html` walkthrough, and `DECISIONS.md`) and Module 16 wires them into a single driver with a small end-to-end test suite (`tests/`, `run_tests.sh`). The `everything_parsing/` track covers its own numbered sequence (01 through 31) independently. This is course material built in public alongside a YouTube series — treat module boundaries as the actual state of the project, not a finished product.

## Exploring / running it

Each module is self-contained and independently buildable:

```sh
cd "module_16 - The Complete Compiler"
make
./mycc path/to/program.c -o program   # shells out to nasm + gcc internally
./run_tests.sh
```

Earlier modules build the same way but expose only the pipeline stage they implement (e.g. Module 02's `main.c` just prints the token stream). Start at Module 01 and read each `tutorial.html` and `DECISIONS.md` in order — later modules assume you understand the trade-offs the earlier ones made and deliberately left unresolved.

The `everything_parsing/` track is independent of the module numbering above; see its own `ROADMAP.md` and `GLOSSARY.md` for its Java/JavaScript build instructions.

## Part of a series

A-finity is the compiler half of a two-part, build-it-from-scratch systems programming series under the **AppSec Education** track — reasoning about software the way an attacker or vulnerability researcher does, from first principles, with no frameworks in between you and the machine. The second half, **theBud OS**, boots and runs the binaries this compiler produces.
