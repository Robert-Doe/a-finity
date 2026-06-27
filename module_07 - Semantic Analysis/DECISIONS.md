# Module 07 Design Decisions

## 1. Two-pass design: collect functions first, then check bodies

We walk the AST twice.  Pass 1 visits every `AST_FUNC` node at the
top level of `AST_PROGRAM` and records the function name and parameter
count in a `FuncEntry` array inside `SemaCtx`.  Pass 2 then walks each
function body and can validate call sites against that table.

The alternative — a single pass — would require every function to be
declared before it is called.  That would break mutually recursive
functions and force a forward-declaration syntax we haven't built.
Two passes give us the same "whole-program" visibility that real C
compilers use without any extra language features.

## 2. Continue checking after errors (collect all errors in one run)

When a semantic error is found we print it immediately, increment
`ctx.errors`, and then **keep going**.  We do not return early from
`check_node`.

The alternative is to stop at the first error.  That's simpler to
implement but very frustrating for users: fix one error, recompile,
see the next error, repeat.  Continuing means a single compilation
run shows every error in the file.  Where one error might cause false
positives downstream (e.g. an undeclared variable also triggers errors
on every use) we accept that trade-off for the benefit of completeness.

## 3. Scope stack in symtab for variable tracking

We use `symtab.h` — a stack of linked-list scopes — to track which
variables are visible at each point in the AST walk.  `symtab_push_scope`
opens a new level; `symtab_pop_scope` closes it and discards all names
defined there.

We open *two* scopes per function: one for the parameter list (pushed
in `check_node` for `AST_FUNC`) and one for the body block (pushed
automatically when `check_node` processes the `AST_BLOCK`).  This means
a local variable with the same name as a parameter is caught as a
duplicate in the body scope, while parameters are visible throughout.

The alternative — a flat hash table — cannot handle nested scopes
(e.g. variables declared inside an `if` block must not be visible after
the block closes).

## 4. How we handle void functions (no return value required)

Our grammar accepts both `int` and `void` as return-type keywords but
the AST does not store the return type — all functions are treated
uniformly.  The semantic checker therefore does **not** verify that a
`void` function avoids returning a value or that an `int` function
always returns one.

This is intentional for our simplified language: every value is `int`,
and control-flow analysis to guarantee a return on all paths is a
significantly more complex phase (it requires building a control-flow
graph).  We leave that for a future module.

## 5. Why we don't do type inference (only int in our language)

Every expression in our language evaluates to `int`.  There are no
other types: no pointers, no arrays, no structs, no `char`, no
`float`.  Because of this uniformity there is nothing to infer —
every expression already has the same type, so type-checking reduces to
"is this name declared?" rather than "do the types on both sides of
this operator match?".

A real type checker would attach a type to each AST node during the
semantic walk (type synthesis) and then verify that operand types are
compatible with operators (type checking).  We skip that machinery
because it would add complexity without teaching anything new given our
single-type constraint.  Module 09 will introduce a simple type system
when we add pointer support.
