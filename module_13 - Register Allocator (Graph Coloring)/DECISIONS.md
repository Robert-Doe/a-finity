# Module 13 — Register Allocator: Design Decisions

## 1. Live Ranges (first_def / last_use) Instead of Full Dataflow Analysis

We track each temporary's live range as a single closed interval `[first_def, last_use]`
rather than running a full iterative dataflow (gen/kill sets, fixed-point loop).

**Why:** The IR produced by our compiler uses explicit temporaries in SSA-like straight-line
sequences within basic blocks. For this class of IR, the interval approximation is exact —
a temp is live from its first definition to its last use, with no gaps. Full dataflow would
be necessary for loop-carried liveness, but we don't need that complexity here. The interval
approach is O(n) per temp, easy to explain, and produces correct results for our IR.

---

## 2. Callee-Saved Registers (rbx, r12–r15) for the Allocatable Pool

We allocate into `rbx, r12, r13, r14, r15` (plus `r10` as a caller-saved scratch).
The first five are callee-saved under the System V AMD64 ABI.

**Why:** Because a function is required to preserve callee-saved registers across calls,
any value we place in `rbx` or `r12`–`r15` will still be there after we call `clamp`,
`dot`, or any other function. If we instead allocated into caller-saved registers like
`rdi`/`rsi`/`rdx`, we would have to spill them to the stack before every `IR_CALL` and
reload them afterward — complicating codegen significantly. Using callee-saved registers
shifts that burden to the callee (us, in the prologue/epilogue), which is simpler to
implement in a single place.

---

## 3. Greedy Coloring in Temp-Number Order

We process temporaries `t0, t1, t2, …` in ascending order and assign each the lowest
available color (register index) not used by an already-colored interfering temp.

**Why:** The IR generator allocates temp numbers in definition order, so processing
by temp number is equivalent to processing by first definition — earlier-defined temps
get first pick of registers. The algorithm is O(n²) in the number of temps but trivially
correct and easy to follow in a tutorial. Chaitin's original algorithm builds a full
interference graph, finds a vertex ordering by simplicial elimination, and achieves
better coloring quality — but requires substantially more code. For our small programs
the greedy approach almost never spills unnecessarily.

---

## 4. Spilling: What It Means and When It Happens

When the greedy coloring step finds that every allocatable register (`NUM_REGS = 6`) is
already held by an interfering temp, the current temp cannot be given a register. It is
**spilled**: assigned a stack slot at `[rbp - N]` instead.

Every instruction that defines or uses a spilled temp gets extra `mov` instructions
inserted by the code generator — a load before use (`mov rax, [rbp-N]`) and a store
after definition (`mov [rbp-N], rax`). This is correct but slower: each load or store
adds a memory access. In practice, spilling cascades when a function has many live
temporaries simultaneously; the more interferences, the more likely a temp is spilled.

---

## 5. Why rax / rcx / rdx / rdi / rsi Are Excluded from the Allocatable Set

These five registers carry fixed roles in the System V AMD64 ABI and in x86-64
instructions:

- `rax` — return value from all function calls; quotient from `idiv`
- `rcx` — used as a scratch operand in our arithmetic sequences; shift count in `shl`/`shr`
- `rdx` — high half of dividend for `idiv`; remainder result; third call argument
- `rdi`, `rsi` — first and second call arguments

Using any of these for temporaries would require the code generator to detect conflicts
at every `IR_CALL`, `IR_DIV`, and `IR_MOD` and spill/reload the affected temp — turning
every arithmetic or call site into a mini register allocator. Excluding them keeps the
emit logic straightforward: `rax`/`rcx`/`rdx` are always free scratch registers for the
code generator to use internally, and `rdi`/`rsi`/`rdx`/`rcx`/`r8`/`r9` are always
free to load arguments into just before a `call`.

---

## 6. Trade-Off vs. Linear Scan Register Allocator

| Property | Graph Coloring (ours) | Linear Scan (LLVM-style) |
|---|---|---|
| Time complexity | O(n²) interference checks | O(n) over sorted intervals |
| Quality | Near-optimal for small n | Good; misses some opportunities |
| Loop handling | Correct but no loop weighting | Handles loops with live-range splitting |
| Implementation size | ~150 lines (regalloc.c) | ~250 lines with split logic |
| Teaching value | High — interference graph is visual | High — interval picture is intuitive |

**Linear scan** (as described by Poletto & Sarkar 1999 and implemented in early LLVM)
sweeps through intervals sorted by start point, expires intervals that ended, and spills
the interval with the furthest end point when all registers are taken. It is O(n log n)
(dominated by the sort) and handles loop-carried live ranges correctly. We chose graph
coloring because the interference graph concept — nodes are temps, edges are conflicts,
colors are registers — maps perfectly to the constraint-satisfaction framing we want
students to understand.
