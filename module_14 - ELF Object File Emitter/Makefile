CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c11
TARGET=mycc14
SRCS=source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c regalloc.c x86enc.c codegen.c elf_emit.c main.c
OBJS=$(SRCS:.c=.o)

.PHONY: all run link clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET) sample.c -o sample14.o

link: run
	gcc -no-pie sample14.o -o sample14_bin && ./sample14_bin; echo "exit: $$?"

clean:
	rm -f $(OBJS) $(TARGET) sample14.o sample14_bin
