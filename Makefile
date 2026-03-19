EXE_EXT =
ifeq ($(OS),Windows_NT)
  EXE_EXT = .exe
endif

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
INCLUDES = -Iinclude

BIN_DIR = bin
TARGET = $(BIN_DIR)/pingutil$(EXE_EXT)

SRCS = src/main.c src/logger.c src/ping_sim.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(BIN_DIR):
	mkdir $(BIN_DIR) 2>NUL || true

$(TARGET): $(BIN_DIR) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	-del /Q *.o 2>NUL || rm -f $(OBJS)
	-del /Q $(BIN_DIR)\\pingutil*.exe 2>NUL || true

