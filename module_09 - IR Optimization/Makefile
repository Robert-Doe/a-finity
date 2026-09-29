CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=c11
TARGET  = optimize
SRCS    = source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c main.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET) sample.c

clean:
	rm -f $(OBJS) $(TARGET)
