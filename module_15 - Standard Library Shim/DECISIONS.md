# Module 15 Design Decisions

## 1. Why `print` is a keyword, not a function declaration

Making `print` a built-in keyword means the user never has to write a forward
declaration like `void print(int x);` before using it. That would require the
compiler to handle function prototypes, a feature we haven't built yet. By
treating `print` as a first-class statement (like `return` or `while`), we
get output capability without any new declaration machinery. The trade-off is
that `print` is now a reserved word and cannot be used as a variable or
function name — acceptable for a teaching compiler, but something a production
language would handle differently (usually via a prelude or standard header).

## 2. How integer-to-string conversion works in `print_int`

The `div` instruction yields a quotient and a remainder simultaneously.
Dividing by 10 extracts the least-significant decimal digit first, so digits
arrive in reverse order (units, tens, hundreds, ...). We store each digit
right-to-left into a stack buffer, walking a pointer leftward. When the
quotient reaches zero we stop. The pointer now sits one position before the
first (most significant) digit; incrementing it gives us the start of the
string. Length is the distance from that pointer to the newline byte at the
fixed end of the buffer. A single `sys_write` call sends the entire string
(digits + newline) to stdout. For negative numbers we negate the value first
and prepend a `-` character the same way.

## 3. Why use `write(2)` directly instead of calling `printf`

`printf` lives in the C standard library (glibc). Linking against glibc
normally requires dynamic linking (`-lpthread`, startup files, etc.).  Our
linker invocation uses `-no-pie` and links only `program.o` and `runtime.o` —
there is no `-lc`. Using the raw `sys_write` syscall (rax=1 on Linux x86-64)
avoids this dependency entirely. The syscall interface is stable across kernel
versions and is always available, making our runtime completely self-contained
with no external dynamic libraries required.

## 4. Why `runtime.asm` is a separate file instead of inlined into the output

If we inlined `print_int` into every compiled `.asm` file, every program
would carry its own copy of the routine. That wastes space and, more
importantly, creates a maintenance burden: fixing a bug in `print_int` would
require recompiling every program. Keeping the runtime in a separate
`runtime.asm` that is assembled once into `runtime.o` and then linked in
means all compiled programs share a single copy. The linker handles
deduplication automatically. This mirrors how real standard libraries work.

## 5. Why we print a newline after each integer

Printing a raw integer with no separator produces an unreadable stream:
`01123581321` looks like a single number. Appending a newline after each
value makes the output human-readable one value per line. A production
I/O library would separate the concerns — `print_int` would write the digits
only, and a separate `print_newline` or `println_int` would add the line
ending — giving the caller full control. For this course module, one unified
behaviour keeps the implementation and tutorial simpler without loss of
understanding.
