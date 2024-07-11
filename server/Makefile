CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -O2
TARGET = aesdsocket
SRC := $(TARGET).c
OBJ := $(SRC:.c=.o)

ifeq ($(ARCH), arm64)
	CFLAGS += -g
endif

.PHONY: all clean

all: $(TARGET)

#rule for compiling .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

#$(TARGET): $(TARGET).c
#	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET) $(OBJ)
