# Makefile for assignment2

CC := $(CROSS-COMPILE)gcc
TARGET := writer
CFLAGS := -Wall -Wextra -O2
SRC := $(TARGET).c
OBJ := $(SRC:.c=.o)

# target to compile the application
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# target to clean the application and object files
clean: 
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
