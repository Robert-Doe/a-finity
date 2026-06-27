CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c11
TARGET=regalloc_demo
SRCS=source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c regalloc.c codegen.c main.c
OBJS=$(SRCS:.c=.o)

.PHONY: all run assemble clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET) sample.c -o sample13.asm && cat sample13.asm

assemble: run
	nasm -f elf64 sample13.asm -o sample13.o && gcc -no-pie sample13.o -o sample13_bin && ./sample13_bin; echo "exit: $$?"

clean:
	rm -f $(OBJS) $(TARGET) sample13.asm sample13.o sample13_bin
