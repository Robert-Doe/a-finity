CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=c11
TARGET  = codegen12
SRCS    = source.c lexer.c symtab.c ast.c parser.c sema.c ir.c opt.c codegen.c main.c
OBJS    = $(SRCS:.c=.o)

# Output assembly file
ASM     = sample.s
# Final linked binary
BIN     = sample_out

.PHONY: all run assemble clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Generate assembly from sample.c
run: $(TARGET)
	./$(TARGET) sample.c $(ASM)

# Assemble and link the generated .s file (requires Linux/WSL with gcc)
assemble: run
	$(CC) -no-pie -o $(BIN) $(ASM)
	./$(BIN); echo "Exit code: $$?"

clean:
	rm -f $(OBJS) $(TARGET) $(ASM) $(BIN)
