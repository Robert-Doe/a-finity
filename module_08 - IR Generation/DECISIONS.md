# Module 08 — IR Design Decisions

## 1. Three-address code instead of a stack machine or direct AST walking

We use three-address code (TAC) rather than a stack machine or direct code generation
from the AST for three reasons:

- **Explicit temporaries make optimisation straightforward.** Each temporary is written
  exactly once in our forward pass, giving a near-SSA property. A later constant-folding
  or dead-code-elimination pass can inspect `src1`/`src2`/`dst` fields directly without
  having to simulate a stack or re-traverse the tree.

- **A flat, ordered list matches the target.** Real machines execute instructions
  linearly. TAC's flat linked list has the same shape, so lowering to assembly in a
  later module is a near-mechanical translation — one IR instruction → one or two
  assembly instructions.

- **Direct AST walking would mix concerns.** Walking the AST while simultaneously
  emitting assembly conflates IR generation, register allocation, and instruction
  selection into one monolithic pass. Splitting those phases at a well-defined IR
  boundary keeps each module small and testable on its own.

## 2. Linked list for instructions instead of an array

`IRInstr` nodes form a singly-linked list (`IRInstr *next`) rather than a
heap-allocated array (`IRInstr **instrs`). The reasons:

- **O(1) append with a tail pointer.** `IRFunc` carries both `head` and `tail`.
  Every `ir_append` links a new node at the tail without traversing the list or
  reallocating memory, giving true O(1) amortised append at the cost of one extra
  pointer per node.

- **No reallocation invalidates pointers.** If we used a dynamic array and it grew
  past its capacity, every existing `IRInstr *` stored elsewhere (e.g. a back-patch
  pointer to a `JUMPZ` we emitted earlier) would be invalidated. With a linked list,
  node addresses are stable forever.

- **Insertion is cheap for future passes.** An optimisation pass that wants to insert
  a `COPY` between two existing instructions can do so by pointer surgery — no shifting
  of array elements required.

## 3. IR_STORE / IR_LOAD for locals instead of direct register mapping

Local variables are accessed through explicit `IR_STORE name = tN` and
`tN = IR_LOAD name` instructions rather than by assigning each local a dedicated
permanent temporary at declaration time. This simplifies Module 08 in two ways:

- **The code generator does not need a symbol-to-register map.** Every read of `x`
  emits `IR_LOAD x`; every write emits `IR_STORE x`. There is no bookkeeping table
  mapping variable names to temporaries, and no risk of stale mappings after a branch.

- **It mirrors memory semantics accurately.** In C, a local variable lives in memory
  (the stack frame). Modelling it as a named memory cell rather than a register makes
  the IR honest about what the hardware will eventually do, and lets a later register-
  allocation pass decide which locals can be promoted to registers and which must stay
  in memory.

## 4. IR_NO_TEMP sentinel (-1) instead of an optional/nullable type

Fields `dst`, `src1`, and `src2` are plain `int` fields; the value `IR_NO_TEMP`
(`-1`) signals "not used" rather than wrapping the integer in a nullable struct or
using a separate boolean flag per field. The trade-offs:

- **Zero is a valid temporary.** `memset(0)` would leave all three fields as `0`,
  which looks like "temporary t0" — silently wrong. Using `-1` as the sentinel means
  a zeroed-out struct is obviously incorrect, which makes bugs visible immediately
  during printing.

- **C has no built-in `Option<int>`.** Introducing a two-field struct `{ bool present;
  int value; }` for every operand would triple the per-field storage and add accessor
  boilerplate throughout `ir.c`. The sentinel is idiomatic for C and equally expressive
  given the constraint that real temporaries are always non-negative.

- **`ir_print` checks it explicitly.** The one place that must distinguish "no temp"
  from "temp 0" — the `IR_RETURN` printer — tests `instr->src1 == IR_NO_TEMP`
  directly, which is both obvious and unsurprising to any C reader.

## 5. Short-circuit && and || are not implemented in Module 08

The `IR_AND` and `IR_OR` instructions evaluate both operands eagerly:

```
left  = gen_expr(f, n->args[0]);
right = gen_expr(f, n->args[1]);
dst   = ir_new_temp(f);
emit IR_AND dst left right
```

True short-circuit semantics require emitting a conditional jump *between* the two
operand evaluations, which introduces new labels and interleaves control-flow with
value computation — the same machinery used by `AST_IF` and `AST_WHILE`. Doing that
cleanly requires the code generator to pass a "true label / false label" pair through
`gen_expr` in addition to returning a result temporary, a significant refactor.

Module 08 defers that work to Module 09, which introduces the concept of "boolean
destination labels" (or a separate `gen_cond` path). For the programs that appear in
Modules 01–08, no `&&` or `||` expression has an operand with a side-effect, so the
eager evaluation produces the same result. The deferral is explicitly noted in `ir.h`
with a comment on `IR_AND` and `IR_OR`.
