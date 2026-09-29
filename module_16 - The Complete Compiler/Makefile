CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=c11
TARGET  = mycc
SRCS    = source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c codegen.c driver.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all runtime test demo clean

all: $(TARGET) runtime

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

runtime: runtime.o

runtime.o: runtime.asm
	nasm -f elf64 runtime.asm -o runtime.o

test: all runtime
	chmod +x run_tests.sh
	./run_tests.sh

demo: all runtime
	./$(TARGET) tests/t5_fibonacci.c -o fib_demo
	./fib_demo
	echo "exit: $$?"

clean:
	rm -f $(OBJS) $(TARGET) runtime.o /tmp/test_bin fib_demo
