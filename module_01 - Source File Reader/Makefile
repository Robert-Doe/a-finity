# Makefile — Module 01: Source file reader
#
# Targets:
#   make        build the 'reader' binary
#   make run    build and run against sample.c
#   make clean  remove all build artefacts

CC     = gcc

# -Wall    : enable all standard warnings
# -Wextra  : enable extra warnings not covered by -Wall
# -Werror  : treat every warning as an error — no warnings slip through
# -std=c11 : compile as C11 (C99 also works for this module)
CFLAGS = -Wall -Wextra -Werror -std=c11

TARGET = reader
SRCS   = source.c main.c
OBJS   = $(SRCS:.c=.o)

.PHONY: all run clean

# Default target: build the binary from object files.
all: $(TARGET)

# Link the two object files into a single executable.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile each .c file into a .o, listing source.h as a dependency so
# that touching source.h triggers recompilation of both .c files.
%.o: %.c source.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build and immediately run against the provided sample file.
run: all
	./$(TARGET) sample.c

# Remove everything 'make' produced; leaves source files untouched.
clean:
	rm -f $(OBJS) $(TARGET)
