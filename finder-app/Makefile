# Makefile for assignment2

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc
TARGET := writer
CFLAGS := -Wall -Wextra -O2
SRC := $(TARGET).c
OBJ := $(SRC:.c=.o)
LDFLAGS :=

ifeq ($(ARCH), arm64)
	CFLAGS += -g
endif

# target to compile the application
all: $(TARGET)

#rule for compiling .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

#rule for compiling .c files	
$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^ 

#rule for install software
install: $(TARGET)
	cp $(TARGET) $(OUT_DIR)

#rule for printing variables	
print-%:
	@echo $* = $($*)

# target to clean the application and object files
clean: 
	rm -f $(TARGET) $(OBJ)

.PHONY: all install print-% clean
