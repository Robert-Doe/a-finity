# Makefile — Module 03: Symbol Table
#
# Builds symtab_demo, which lexes a C source file and populates a
# symbol table via token-pattern matching, then dumps the results.
# Requires: gcc (tested with GCC 11+)

CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11

TARGET = symtab_demo
SRCS   = source.c lexer.c symtab.c main.c
OBJS   = $(SRCS:.c=.o)   # derive object file names from source file names

# default target: build the executable
all: $(TARGET)

# link object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# compile each .c file to a .o; $< is the source, $@ is the output
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# run the demo against the bundled sample input
run: $(TARGET)
	./$(TARGET) sample.c

# remove all build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# phony targets do not correspond to actual files
.PHONY: all run clean
