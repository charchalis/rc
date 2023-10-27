# Makefile for myprogram

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall

# Source file
SRC = main.c applicationLayer.c linkLayer.c

# Output file
OUTPUT = test 

PORT10 = /dev/ttyS10
PORT11 = /dev/ttyS11

WRITE = tx
READ = rx

FILE = pinguim.gif

# Default target
all: $(OUTPUT)


tx: compile 
	./$(OUTPUT) $(PORT10) $(WRITE) $(FILE)

rx: compile
	./$(OUTPUT) $(PORT11) $(READ) $(FILE)


# Compile the source file
compile: $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC)

double: compile write

# Clean up compiled files
clean:
	rm -f $(OUTPUT)

