# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Executable name
TARGET = mysh

# Source files
SRC = mysh.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Rule for compiling .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJ) $(TARGET)

# Run the shell (if you want to test the shell directly after building)
run: $(TARGET)
	./$(TARGET)

# For batch mode testing (if you want to run with a file)
batch_run: $(TARGET)
	./$(TARGET) testfile.txt

.PHONY: all clean run batch_run
