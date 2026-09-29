# Module 14 Design Decisions

## 1. Why ET_REL (relocatable) not ET_EXEC

We emit `ET_REL` and let the linker (`gcc -no-pie`) handle symbol resolution and final memory layout. Building `ET_EXEC` directly requires knowing exact load addresses for every symbol and resolving all external references (libc, etc.) ourselves. A relocatable `.o` file lets the linker do that heavy lifting — which is exactly what linkers are designed for.

## 2. Why we don't use RELA relocations for our own function calls

All functions in a single compiled file end up concatenated in the same `.text` section of the same `.o`. When the linker processes the object it knows the relative offset between any two functions in that section and can patch the `call rel32` directly. External calls (e.g., `printf` from libc) would require `.rela.text` entries, but our subset of C does not call external symbols yet — that is deferred to Module 15.

## 3. How .strtab and .shstrtab differ

`.strtab` holds null-terminated name strings for *symbols* recorded in `.symtab`; the `st_name` field of each `Elf64_Sym` is a byte offset into `.strtab`. `.shstrtab` holds null-terminated name strings for *sections* themselves; the `sh_name` field of each `Elf64_Shdr` is a byte offset into `.shstrtab`. They are separate because they are indexed by different ELF structures and can have independent lifetimes — a stripped binary may discard `.strtab` while keeping `.shstrtab` for section identification.

## 4. Why the 1 MB buffer limit

Simplicity. Real object file writers use `mmap` or streaming I/O to handle arbitrarily large output. For a learning compiler that compiles small programs (hundreds of IR instructions at most), 1 MB is a safe upper bound. Hitting the limit aborts with a clear error rather than silently corrupting the output.

## 5. Why <elf.h> instead of manually defining ELF structs

`<elf.h>` is part of glibc and is present on every Linux and WSL system. Its struct definitions (`Elf64_Ehdr`, `Elf64_Shdr`, `Elf64_Sym`, etc.) are standardized by the System V ABI and are correct by construction. Redefining them manually would risk transcription errors across the 50+ fields involved and make the code harder to compare against the official specification.
