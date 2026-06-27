CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
TARGET = sema
SRCS   = source.c lexer.c symtab.c ast.c parser.c sema.c main.c
OBJS   = $(SRCS:.c=.o)

.PHONY: all run run-bad clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependency hints so headers trigger rebuilds
source.o:  source.c  source.h
lexer.o:   lexer.c   lexer.h   token.h   source.h
symtab.o:  symtab.c  symtab.h
ast.o:     ast.c     ast.h
parser.o:  parser.c  parser.h  lexer.h   ast.h   token.h   source.h
sema.o:    sema.c    sema.h    ast.h     symtab.h
main.o:    main.c    source.h  lexer.h   parser.h ast.h     sema.h

run: all
	./$(TARGET) sample.c

run-bad: all
	./$(TARGET) sample_bad.c

clean:
	rm -f $(OBJS) $(TARGET)
