# Module 12 — Design Decisions

## 1. Why must the stack be 16-byte aligned before a `call` instruction?

The System V AMD64 ABI mandates that `rsp` is a multiple of 16 **immediately before** any `call` instruction executes. This rule exists because some instructions (notably SSE/AVX vector loads/stores like `movaps`) require 16-byte-aligned memory operands and will raise a general-protection fault if misaligned. The C runtime and many library functions rely on this alignment without checking.

When `call` executes it pushes an 8-byte return address, making `rsp` 8-byte aligned at the callee's entry. The callee's prologue then does `push rbp` (another 8 bytes), restoring 16-byte alignment before any computation happens.

Our `compute_frame_size` function ensures `(frame_size + 8) % 16 == 0`: the `+8` accounts for the `push rbp` that immediately precedes our `sub rsp, N`.

---

## 2. Where do parameters live after function entry, and why do we copy them to stack slots immediately?

At the very first instruction of a called function, the first six integer arguments reside in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`. These are **caller-saved** registers: any `call` instruction we emit later will be free to clobber them (the called function is not required to restore them).

We immediately copy (spill) all incoming parameters to their named stack slots during the prologue:

```asm
movq  %rdi, -8(%rbp)    # param 'x'
movq  %rsi, -16(%rbp)   # param 'lo'
movq  %rdx, -24(%rbp)   # param 'hi'
```

After this spill, parameters are accessed the same way as any other named local — via `IR_LOAD` / `IR_STORE` — and the argument registers are free to be reused for outgoing calls. Without this spill, a nested call would overwrite the argument registers and silently corrupt the parameter values.

---

## 3. Why do we not support more than 6 function arguments?

The System V ABI passes the first 6 integer arguments in registers. Arguments 7 and beyond are passed **on the stack**, pushed in right-to-left order by the caller before the `call` instruction, at positive offsets from the callee's `rbp` (starting at `rbp+16`). Supporting this requires:

- The caller to reserve extra stack space and push excess arguments.
- The callee to read those arguments from `rbp+16`, `rbp+24`, etc.
- Careful alignment accounting because the extra pushes change `rsp`.

This is well-defined in the ABI but adds significant complexity. Module 12 focuses on understanding the core calling convention. The 6-argument limit is enforced at codegen time with an explicit error message, and the tutorial explains what happens beyond 6 args.

---

## 4. What is the difference between callee-saved and caller-saved registers?

**Caller-saved registers** (also called *volatile* or *scratch* registers): `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`, `r9`, `r10`, `r11`. A function may freely modify these without saving them first. If the **caller** has a live value in one of these registers across a function call, the caller must save and restore it itself.

**Callee-saved registers** (also called *non-volatile* or *preserved* registers): `rbx`, `rbp`, `r12`, `r13`, `r14`, `r15`. If a function wants to use one of these registers, it **must** save the original value (e.g., with `push`) at function entry and restore it (e.g., with `pop`) before returning. This guarantees the caller's values in these registers are unchanged after the call returns.

In our compiler we spill all temporaries to the stack and never allocate temporaries to registers, so we never use `rbx`/`r12`-`r15` and never need to save/restore them.

---

## 5. Why do we subtract a rounded-up value from `rsp` rather than the exact amount?

We need two properties simultaneously:

1. **Enough space**: at least `total_slots * 8` bytes for all temporaries and locals.
2. **16-byte alignment**: `(frame_size + 8) % 16 == 0` so that `rsp` is 16-byte aligned throughout the function body (which guarantees it is aligned before any `call`).

The exact slot count rarely satisfies both constraints at once, so we round up to the nearest satisfying value. Adding 8 bytes wastes at most one stack slot (8 bytes) per function — an entirely acceptable trade-off for correctness. The alternative — computing the exact minimum — would require solving a small modular arithmetic problem for every function, with no practical benefit.

---

## 6. How do we verify ABI compliance? (Link with `gcc -no-pie`)

The easiest verification method is to link our generated assembly with gcc and call our compiled functions from a C test harness (or vice versa):

```sh
# Compile the sample
./codegen12 sample.c sample.s

# Link with gcc — -no-pie avoids position-independent-executable complications
gcc -no-pie -o sample_out sample.s

# Run and check exit code
./sample_out
echo "Exit code: $?"   # expected: 15
```

If our calling convention is wrong the program will either crash (segfault from a misaligned stack), produce the wrong answer (wrong registers used), or loop forever (stack frame corrupted). A correct exit code of 15 is strong evidence the ABI is followed correctly.

Additional checks:
- `objdump -d sample_out` — inspect the disassembly.
- `gdb ./sample_out` with `break clamp` — verify register values at function entry match what we loaded.
- Compile a C version of `clamp` with `gcc -O0` and diff the assembly structure.
