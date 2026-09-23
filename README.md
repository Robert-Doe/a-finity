# A-finity

I've used `gcc -O2` for years the way most people use it: type the command, trust the output, move on. That's fine until you're trying to reason about a miscompilation bug, an ABI mismatch, or a code-generation flaw in a real vulnerability report, and you realize you've never actually watched a compiler turn source text into a working binary. You've only ever consumed the result. So I built one myself, from the raw bytes of a source file all the way to a linkable x86-64 ELF object that actually runs.

This is the first half of a two-part systems series. The second half, `theBud OS`, boots and runs the exact binaries this compiler emits.

## What I actually built

I'm not trying to reproduce GCC, that would be a waste of both our time. What I wanted was every stage a real compiler front end and back end actually needs: a lexer, a symbol table, a recursive-descent parser, semantic analysis, an intermediate representation, optimization passes, register allocation, x86-64 code generation, and an ELF emitter, all written by hand in C, so every decision I made, and every corner I cut to keep a given module tractable, is visible and written down.

That's also where the security angle comes in. Most memory-corruption and logic-confusion bugs in real compilers and interpreters live exactly in these stages: lexical edge cases, symbol resolution gone wrong, calling-convention mismatches, unchecked buffer math during code emission. You can't reason like a vulnerability researcher about a code-generation bug if you've never generated code yourself.

Every module ships a `DECISIONS.md` that records not just what I built, but why, including the production trade-off I was deliberately punting on. Things like "linear-scan symbol table for now, hash table once Module 07 actually needs it," or "deep-copy semantics for now, arena allocation once lifetimes start to matter." Read those files in order and you're basically watching a compiler-construction course unfold in real time.

There's also a companion deep dive, `everything_parsing/`, which rebuilds the parsing theory this whole compiler leans on: regular languages, DFA and NFA construction, LL(1) table-driven parsing, shift-reduce and LR(0) items. I built it with a working Java implementation and a working JavaScript implementation side by side for every single concept, aiming for the same rigor as a real formal PL-principles course (I had ASU's CSE 340 and the Dragon Book in mind) rather than a hand-wavy overview.

## Module map

| # | Module | What it builds |
|---|--------|-----------------|
| 01 | [Source File Reader](./module_01%20-%20Source%20File%20Reader) | Load a source file into a single flat buffer, the foundation every later stage indexes into |
| 02 | [Lexer / Tokeniser](./module_02%20-%20Lexer%20-%20Tokeniser) | Hand-written scanner: keywords, identifiers, operators, `TOK_ERROR` recovery instead of `exit()` |
| 03 | [Symbol Table](./module_03%20-%20Symbol%20Table) | Flat linear-scan symbol storage, the deliberate limitation Module 07 will break and fix |
| 04 | [Parser, Expressions](./module_04%20-%20Parser%20-%20Expressions) | Recursive-descent precedence climbing, left recursion rewritten as iteration |
| 05 | [Parser, Statements](./module_05%20-%20Parser%20-%20Statements) | Two-token lookahead to tell an assignment apart from an expression statement |
| 06 | [Parser, Functions](./module_06%20-%20Parser%20-%20Functions) | Function definitions, parameters, and the AST shapes that carry them |
| 07 | [Semantic Analysis](./module_07%20-%20Semantic%20Analysis) | Two-pass checking, collect signatures first, then validate bodies, so mutual recursion works |
| 08 | [IR Generation](./module_08%20-%20IR%20Generation) | Lowering the AST down to three-address code |
| 09 | [IR Optimization](./module_09%20-%20IR%20Optimization) | Local constant folding, copy propagation, dead-code elimination, each as its own separable pass |
| 10 | [x86-64 Code Generation](./module_10%20-%20x86-64%20Code%20Generation) | Naive stack-slot codegen, every temporary gets a fixed `[rbp - N]` slot |
| 11 | [Code Generation, Control Flow](./module_11%20-%20Code%20Generation%20-%20Control%20Flow) | Branches, loops, and idiomatic `test`/`jz` control flow |
| 12 | [Function Calls & the System V AMD64 Calling Convention](./module_12%20-%20Function%20Calls%20%26%20the%20System%20V%20AMD64%20Calling%20Convention) | Stack alignment, argument register spilling, ABI-correct prologues |
| 13 | [Register Allocator (Graph Coloring)](./module_13%20-%20Register%20Allocator%20\(Graph%20Coloring\)) | Live-range analysis and graph-coloring allocation into callee-saved registers |
| 14 | [ELF Object File Emitter](./module_14%20-%20ELF%20Object%20File%20Emitter) | A hand-rolled `ET_REL` ELF64 writer: sections, symtab, strtab, relocations |
| 15 | [Standard Library Shim](./module_15%20-%20Standard%20Library%20Shim) | A `print` built-in and hand-written integer-to-string conversion, no libc dependency |
| 16 | [The Complete Compiler](./module_16%20-%20The%20Complete%20Compiler) | The end-to-end driver (`nasm` and `gcc` used as the linker) with a real test suite |
| - | [everything_parsing/](./everything_parsing) | A standalone parsing-theory deep dive: regex to DFA/NFA to Thompson/subset construction to LL(1) and LR(0) parsing, in parallel Java and JavaScript implementations |

## Tech stack

C11 for the compiler itself, the lexer, parser, semantic analysis, IR, codegen, and ELF emitter. x86-64 assembly in NASM syntax for the runtime shim and the code the compiler actually emits. GNU Make, one file per module. Java and JavaScript (Node, `.mjs`) as dual reference implementations for the `everything_parsing` theory track. The target is x86-64 ELF (`ET_REL`), linked with `gcc -no-pie` acting as the linker front end.

## Where this stands

Still in progress, and I mean that honestly. Modules 01 through 16 each have a working, buildable stage of the pipeline (a `Makefile`, source, a `tutorial.html` walkthrough, and `DECISIONS.md`), and Module 16 wires all of it into a single driver with a small end-to-end test suite (`tests/`, `run_tests.sh`). The `everything_parsing/` track runs its own numbered sequence, 01 through 31, on its own schedule. This is course material I'm building in public alongside a YouTube series, so treat the module boundaries as exactly where the project actually is, not a finished, polished product.

## Exploring or running it

Every module is self-contained and buildable on its own:

```sh
cd "module_16 - The Complete Compiler"
make
./mycc path/to/program.c -o program   # shells out to nasm + gcc internally
./run_tests.sh
```

Earlier modules build the same way, they just expose only the pipeline stage they implement (Module 02's `main.c`, for instance, just prints the token stream). Start at Module 01 and read each `tutorial.html` and `DECISIONS.md` in order. Later modules assume you already understand the trade-offs the earlier ones made and deliberately left unresolved.

The `everything_parsing/` track has its own numbering, independent of the module list above. See its own `ROADMAP.md` and `GLOSSARY.md` for the Java and JavaScript build instructions.

## Part of a series

A-finity is the compiler half of a two-part, build-it-from-scratch systems programming series I call **AppSec Education**: reason about software the way an attacker or vulnerability researcher does, from first principles, with no framework standing between you and the machine. The second half, **theBud OS**, boots and runs the binaries this compiler produces.
