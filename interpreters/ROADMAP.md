# Interpreters — ROADMAP

> **Status: PLANNED.** Phase 3 of A-finity. Starts once `everything_parsing/`
> reaches Module 44 (Capstone D), because every module here runs programs that
> have already been scanned, parsed and checked by that track.

The compiler track (`../module_01 … module_16`) turns source into machine code
ahead of time. The parsing track (`../everything_parsing/`) builds a front end
for **Ajoda** that ends in a checked AST. This phase answers the remaining
question: **how do you run that AST directly?** It goes from the simplest
possible interpreter to a bytecode virtual machine with a garbage collector,
measuring what each step buys.

Same rules as the parsing track: every module ships a **working Java
implementation and a working JavaScript implementation, side by side**, with
byte-identical output checked against a golden file, and a `tutorial.html` in
the shared design system.

---

## Reference basis

| Source | Used for |
|--------|----------|
| Nystrom, *Crafting Interpreters* | Overall arc from tree-walker to bytecode VM; environments, closures and upvalues; mark-sweep GC |
| Abelson & Sussman, *SICP* ch. 4–5 | The evaluator as a program; environment model; register-machine view of evaluation |
| Ertl & Gregg, "The Structure and Performance of Efficient Interpreters" | Dispatch techniques and why they matter |
| Jones, Hosking & Moss, *The Garbage Collection Handbook* | Mark-sweep, copying, and generational collection |
| Würthinger et al., "One VM to Rule Them All" (Truffle) | Self-specializing AST interpreters and the path to a JIT |
| **Ajoda language spec** | The language every module runs, extended in Part III with closures and heap values |

---

## The phase at a glance

| Part | Theme | Modules | Capstone |
|------|-------|---------|----------|
| I | Tree-walking interpreters | I01–I05 | **E — Ajoda tree-walker** runs every Ajoda test program |
| II | Bytecode and virtual machines | I06–I11 | **F — Ajoda VM** runs the same programs, cross-checked against E |
| III | Memory and performance | I12–I15 | **G — Ajoda REPL** with both engines, a GC, and a profiler |

---

## Part I — Tree-Walking Interpreters  · *Capstone E: Module I05*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| I01 | Values and the Evaluation Loop | Proves an expression evaluator is one recursive function over the AST, and pins down Ajoda's `i64` / `bool` / `f64` value representation in two host languages (Java `long` vs. JS `BigInt`). | `i01-values-and-eval/` | Not started |
| I02 | Environments, Blocks and Assignment | Proves a chain of environments gives lexical scope at run time, matching the scope stack the semantic checker used at compile time. | `i02-environments/` | Not started |
| I03 | Functions, Calls and Returns | Proves calls need a fresh environment per activation, and compares two ways to implement `return` (host exceptions vs. a completion signal) by cost and clarity. | `i03-functions-and-calls/` | Not started |
| I04 | Runtime Errors and Stack Traces | Proves division by zero, `i64` overflow policy and runaway recursion can all be reported with `line:col` and an Ajoda-level stack trace, instead of a host-language crash. | `i04-runtime-errors/` | Not started |
| I05 | The Ajoda Tree-Walker  *(Capstone E)* | Proves the full pipeline: token spec (Capstone A) → grammar (Capstone B) → checks (C, D) → evaluation, running every Ajoda test program with golden output. | `i05-ajoda-tree-walker/` | Not started |

## Part II — Bytecode and Virtual Machines  · *Capstone F: Module I11*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| I06 | What a Tree-Walker Costs | Proves, by measurement, where a tree-walker spends its time (node dispatch, environment lookup, allocation) on the same programs. | `i06-tree-walk-cost/` | Not started |
| I07 | Designing a Stack-Machine Instruction Set | Proves a small set of stack instructions (constants, locals, arithmetic, compare, jump, call, return) is enough for all of Ajoda. | `i07-instruction-set/` | Not started |
| I08 | Compiling the AST to Bytecode | Proves one pass over the checked AST emits correct bytecode, with a constant pool, resolved local slots, and jump backpatching for `if` / `while`. | `i08-bytecode-compiler/` | Not started |
| I09 | The VM Loop and Dispatch | Proves a fetch-decode-execute loop runs the bytecode, and measures `switch` dispatch against a table of handlers in both host languages. | `i09-vm-loop/` | Not started |
| I10 | Calls and Frames in the VM | Proves a single value stack plus a frame stack implements calls, recursion and returns with no host recursion at all. | `i10-vm-frames/` | Not started |
| I11 | The Ajoda VM  *(Capstone F)* | Proves the VM and the tree-walker produce identical output on every test program, plus a disassembler and a single-step debugger. | `i11-ajoda-vm/` | Not started |

## Part III — Memory and Performance  · *Capstone G: Module I15*

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| I12 | Closures and Upvalues | Proves nested functions that capture variables need captured cells that outlive their frame, and implements them in both engines (an Ajoda extension). | `i12-closures-upvalues/` | Not started |
| I13 | Heap Values and Garbage Collection | Proves mark-sweep reclaims unreachable strings and arrays (an Ajoda extension), with the VM stack and globals as roots, and measures pause times. | `i13-garbage-collection/` | Not started |
| I14 | Making It Faster | Proves compile-time constant folding, superinstructions and inline caching each cut measured run time, and shows where each one stops paying off. | `i14-optimizations/` | Not started |
| I15 | The Ajoda REPL  *(Capstone G)* | Proves an interactive loop can scan, parse, check and run one line at a time while keeping definitions, with a switch between engines and a built-in profiler. | `i15-ajoda-repl/` | Not started |

---

## Appendix (optional)

| # | Module | What it proves | Directory | Status |
|---|--------|----------------|-----------|--------|
| IX1 | Closure Compilation | Proves turning each AST node into a host-language closure once, then calling closures, beats re-walking the tree. The simplest step toward a JIT. | `appendix/ix1-closure-compilation/` | Not started |
| IX2 | Self-Specializing AST Interpreters | Proves nodes that rewrite themselves based on observed types (Truffle-style) remove most dynamic checks. | `appendix/ix2-self-specializing/` | Not started |
| IX3 | Copying and Generational GC | Proves a copying collector compacts the heap and a nursery makes short-lived allocation cheap. | `appendix/ix3-copying-generational-gc/` | Not started |
| IX4 | Interpreter Security | Proves where interpreters go wrong in practice: unchecked bytecode, type confusion in the VM, GC bugs that free live objects. Includes a bytecode verifier for the Ajoda VM. | `appendix/ix4-interpreter-security/` | Not started |

---

## How this connects to the other two tracks

| Question | Where it's answered |
|----------|---------------------|
| How is source text turned into tokens and trees? | `../everything_parsing/`, Capstones A and B |
| How are names and types checked? | `../everything_parsing/`, Capstones C and D |
| How is a checked program run *directly*? | **this phase**, Parts I–II |
| How is a program turned into native code *ahead of time*? | `../module_08 … module_16` (IR, register allocation, x86-64, ELF) |
| Interpreted vs. compiled: what's the actual difference in cost? | I06 and I14 here, compared with the compiler track's output |

---

## Build rules (same as the parsing track)

- Java 17+ and Node 20+, zero third-party libraries, no build tool.
- Each module: `java/`, `js/`, `fixtures/`, `expected/`, `run.md`,
  `DECISIONS.md`, `tutorial.html`.
- One module fully finished (code → verified real output → DECISIONS.md →
  tutorial.html) before the next.
- Ajoda test programs live in one shared `fixtures/programs/` folder so every
  engine runs exactly the same inputs.
