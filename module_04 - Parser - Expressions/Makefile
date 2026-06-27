TARGET = parser_expr
SRCS   = source.c lexer.c symtab.c ast.c parser.c main.c
OBJS   = $(SRCS:.c=.o)
CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
