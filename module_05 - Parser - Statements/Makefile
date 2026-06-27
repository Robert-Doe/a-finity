# Makefile for Module 05 — Parser: Statements
# Builds the 'parser' executable from all C source files.

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=c11
TARGET  = parser
SRCS    = source.c lexer.c symtab.c ast.c parser.c main.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 'run' feeds sample.c through the parser and prints the AST
run: $(TARGET)
	./$(TARGET) sample.c

clean:
	rm -f $(TARGET) $(OBJS)
