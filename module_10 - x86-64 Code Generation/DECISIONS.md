# Module 10 — Design Decisions

## 1. Naive Stack Allocation (Each Temp = 8 Bytes on the Stack)

**Decision:** Every IR temporary `t_i` maps to a fixed slot `[rbp - (i+1)*8]`.
Named local variables get slots starting at `[rbp - (MAX_TEMP_SLOTS + slot + 1)*8]`.

**Why:** Real register allocators (graph coloring, linear scan) are complex to implement
correctly and are the topic of an entire compiler course module. For a beginner-focused
educational compiler the simplest strategy that *works* is the right choice. Every
temporary lives in memory; loads and stores are explicit. This makes the generated
assembly easy to read and trace: you can always find a value at a predictable address.

**Trade-off:** We allocate space for `MAX_TEMP_SLOTS` (256) temporaries regardless of
how many the function actually uses, wasting stack space. Production compilers compute
the exact live range of each temp and pack them tightly.

---

## 2. rbp-Relative Addressing

**Decision:** We use `rbp` (frame base pointer) to address locals, *not* `rsp` (stack
pointer).

**Why:** `rsp` changes whenever we push/pop or call functions. Using `rsp`-relative
addresses would require recalculating all offsets after every stack movement.
`rbp` is saved at function entry (`push rbp; mov rbp, rsp`) and never moves during
the function body, giving us a stable reference for the entire function. This is the
classic calling-convention approach and what debuggers (GDB, LLDB) expect for
unwinding stack frames.

---

## 3. Why `cqo` Before `idiv`

**Decision:** We emit `cqo` immediately before every `idiv` instruction.

**Why:** The x86-64 `idiv rcx` instruction divides the 128-bit value `rdx:rax` by
the operand. If we only set `rax` to the dividend, `rdx` contains garbage, leading
to incorrect results or a divide exception. `cqo` (Convert Quadword to Octaword)
sign-extends `rax` into `rdx:rax`, properly forming the 128-bit dividend for signed
division. Forgetting `cqo` is one of the most common bugs in hand-written x86-64
assembly.

---

## 4. Why `section .note.GNU-stack`

**Decision:** Every generated assembly file ends with:
```
section .note.GNU-stack noalloc noexec nowrite progbits
```

**Why:** On Linux, the GNU toolchain marks the stack as non-executable by default
for security (NX/DEP protection). If *any* object file in the link lacks this
marker, the linker assumes the stack needs to be executable and emits a warning.
By explicitly including this section with `noexec`, our generated object files
play nicely with the OS security model and suppress linker warnings. Without it,
running `gcc -no-pie sample.o -o sample_bin` prints:
`warning: .note.GNU-stack is missing`.

---

## 5. System V AMD64 ABI Summary

**Decision:** We follow the System V AMD64 ABI (the standard calling convention on
Linux) for all function calls.

**Key rules we implement:**
| Item | Rule |
|------|------|
| Integer/pointer args 1–6 | `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9` |
| Further args | Pushed on stack, right-to-left |
| Return value (integer) | `rax` |
| Callee-saved registers | `rbp`, `rbx`, `r12`–`r15` (we only save `rbp`) |
| Caller-saved registers | `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`–`r11` |
| Stack alignment | `rsp` must be 16-byte aligned at the point of `call` |

**Why this matters:** The `add` function in `sample.c` receives its arguments
in `rdi` and `rsi`. Our code generator emits `mov rdi, rax` / `mov rsi, rax`
before the `call add` instruction. After `call`, the return value is in `rax`,
which we immediately spill to the caller's stack frame.

---

## 6. Outputting NASM Text Assembly vs. Binary Machine Code

**Decision:** We output NASM-syntax text assembly, *not* raw machine code bytes.

**Why:**
1. **Readability / Debuggability** — Students can open `sample.asm` and read every
   instruction. This is invaluable for understanding what the compiler did and why.
2. **Correctness** — Encoding x86-64 machine code correctly (handling REX prefixes,
   ModRM bytes, SIB bytes, displacement sizes) is extremely complex. NASM handles
   all of this for us.
3. **Portability** — The same text output works on any Linux/WSL machine with NASM
   installed, regardless of whether it's a native x86-64 or running under emulation.
4. **Educational value** — The two-step process (compile → assemble → link) mirrors
   the real-world toolchain and teaches students how the pieces fit together.

**Trade-off:** Requires NASM to be installed separately. For a production compiler
you would want to emit ELF object files directly (using a library like `libbfd` or
writing the ELF header by hand), but that complexity is not appropriate for Module 10.
