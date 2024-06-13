# Makefile for assignment2

CC := $(CROSS-COMPILE)gcc
LD := $(CROSS-COMPILE)gcc
TARGET := writer
CFLAGS := -Wall -Wextra -O2
SRC := $(TARGET).c
OBJ := $(SRC:.c=.o)
LDFLAGS :=



# target to compile the application
all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
print-%:
	@echo $* = $($*)

# target to clean the application and object files
clean: 
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean print-%
