CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
TARGET = codegen
SRCS   = source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c codegen.c main.c
OBJS   = $(SRCS:.c=.o)

# ------------------------------------------------------------------ #
# Primary targets                                                      #
# ------------------------------------------------------------------ #

.PHONY: all clean run assemble

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ------------------------------------------------------------------ #
# Run: compile sample.c and print the generated assembly              #
# ------------------------------------------------------------------ #

run: all
	./$(TARGET) sample.c -o sample.asm
	cat sample.asm

# ------------------------------------------------------------------ #
# Assemble + link + execute (requires NASM on Linux/WSL)             #
# ------------------------------------------------------------------ #

assemble: run
	nasm -f elf64 sample.asm -o sample.o
	gcc -no-pie sample.o -o sample_bin
	./sample_bin; echo "exit: $$?"

# ------------------------------------------------------------------ #
# Clean                                                                #
# ------------------------------------------------------------------ #

clean:
	rm -f $(OBJS) $(TARGET) sample.asm sample.o sample_bin

# ------------------------------------------------------------------ #
# Header dependencies                                                  #
# ------------------------------------------------------------------ #

source.o:  source.h
lexer.o:   lexer.h source.h token.h
symtab.o:  symtab.h
ast.o:     ast.h token.h
parser.o:  parser.h lexer.h ast.h
sema.o:    sema.h ast.h symtab.h
ir.o:      ir.h ast.h
opt.o:     opt.h ir.h
codegen.o: codegen.h ir.h
main.o:    source.h lexer.h parser.h sema.h ir.h opt.h codegen.h
