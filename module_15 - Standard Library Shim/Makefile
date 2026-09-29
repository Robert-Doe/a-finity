CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c11
TARGET=mycc15
SRCS=source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c codegen.c main.c
OBJS=$(SRCS:.c=.o)

.PHONY: all run assemble clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET) sample15.c -o sample15.asm

assemble: run
	nasm -f elf64 sample15.asm -o sample15_prog.o
	nasm -f elf64 runtime.asm -o runtime.o
	gcc -no-pie sample15_prog.o runtime.o -o sample15_bin
	./sample15_bin

clean:
	rm -f $(OBJS) $(TARGET) sample15.asm sample15_prog.o runtime.o sample15_bin
