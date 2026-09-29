# Module 09 — Design Decisions

## 1. Why three separate passes instead of one combined pass?

Each pass has a single, clearly defined responsibility. Combining them into
one giant loop would make the code harder to read, test, and extend. Separate
passes also compose cleanly: you can add, remove, or reorder passes without
touching the other pass implementations. The performance cost of extra linear
scans through the instruction list is negligible for the function sizes we
compile at this stage.

## 2. Why local constant folding only (no inter-procedural)?

Inter-procedural constant folding — knowing, for example, that `square(7)`
must return 49 — requires either function inlining or a separate inter-
procedural analysis pass. Both are significant additions that are better
introduced in a dedicated module. Within a single function our local approach
is sound and complete: every arithmetic instruction whose operands are
provably constant at compile time will be folded. Calls to other functions
remain opaque because the callee might read global state, call further
functions, or have side effects we cannot reason about without a full call-
graph analysis.

## 3. Why scan the whole instruction list for copy propagation instead of
   using a reaching-definitions set?

A reaching-definitions set is the "correct" data-flow solution, but it
requires building a control-flow graph and iterating to a fixed point — a
significant increase in code complexity. For the subset of IR this compiler
currently generates, a single forward scan is sufficient: our IR generator
is essentially SSA-like (each temporary is written exactly once in a forward
pass), so a temp's definition always dominates its uses in straight-line
code. The single-scan approach correctly handles all patterns our IR
generator emits and is simple enough to read and debug in one sitting.

## 4. Limitations of dead-code removal (only after unconditional jumps)

The dead-code pass only removes instructions in the specific pattern:

    IR_JUMP  <label>
    <unreachable instructions>   ← removed
    IR_LABEL <label>

More general dead-code elimination — removing entire basic blocks that have
no predecessors in the control-flow graph — requires computing the graph and
doing a reachability analysis from the function entry point. That is left for
a later module. The current pass still handles the most common real-world
case: code that appears after a `return` statement inside an `if` branch,
which the IR generator emits as instructions between a JUMP and the
following LABEL.

## 5. Order of passes: fold first, then copy propagation, then dead code

Running constant folding before copy propagation means that folded constants
immediately appear as IR_ICONST instructions. Copy propagation can then
forward those constants directly into subsequent uses, eliminating
intermediate temporaries that would otherwise survive into the code
generator. Dead code is run last because the dead regions it removes may
contain instructions that copy propagation would otherwise try to process,
and because constant folding never creates new unreachable code — it only
replaces existing instructions.
