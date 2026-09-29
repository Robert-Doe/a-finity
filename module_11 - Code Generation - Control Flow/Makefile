TARGET = codegen11
SRCS   = source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c codegen.c main.c
OBJS   = $(SRCS:.c=.o)

CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g

.PHONY: all run assemble clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Run: compile sample.c → sample11.asm and display the assembly ──────────
run: $(TARGET)
	./$(TARGET) sample.c -o sample11.asm && cat sample11.asm

# ── Assemble, link, and run the generated binary ───────────────────────────
# Requires: nasm, gcc
# The exit code of sample11_bin is a+b = abs_val(-5) + sum_to(10) = 5+45 = 50
assemble: sample11.asm
	nasm -f elf64 sample11.asm -o sample11.o
	gcc -no-pie sample11.o -o sample11_bin
	./sample11_bin; echo "exit code: $$?"

# ── Clean build artifacts ──────────────────────────────────────────────────
clean:
	rm -f *.o $(TARGET) sample11.asm sample11_bin

# ── Dependency tracking (header changes trigger recompile) ─────────────────
source.o:  source.c  source.h
lexer.o:   lexer.c   lexer.h   token.h
symtab.o:  symtab.c  symtab.h
ast.o:     ast.c     ast.h
parser.o:  parser.c  parser.h  lexer.h ast.h
sema.o:    sema.c    sema.h    ast.h   symtab.h
ir.o:      ir.c      ir.h      ast.h
opt.o:     opt.c     opt.h     ir.h
codegen.o: codegen.c codegen.h ir.h
main.o:    main.c    source.h  parser.h sema.h ir.h opt.h codegen.h
