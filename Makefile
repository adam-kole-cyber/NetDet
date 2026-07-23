CC := gcc
BASE_CFLAGS := -Wall -Wextra -pedantic -std=c11 \
               -D_POSIX_C_SOURCE=200809L \
               -Iinclude

DEBUG_CFLAGS := -g3 -O0 \
                -fsanitize=address,undefined,leak \
                -fno-omit-frame-pointer \
                -DDEBUG

BASE_LDFLAGS := -lncursesw -lpthread
DEBUG_LDFLAGS := -fsanitize=address,undefined,leak

SRC_DIR := src
OBJ_DIR := build/obj
BIN := NetDet

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))


.PHONY: all debug clean


all: CFLAGS=$(BASE_CFLAGS)
all: LDFLAGS=$(BASE_LDFLAGS)
all: $(BIN)


debug: CFLAGS=$(BASE_CFLAGS) $(DEBUG_CFLAGS)
debug: LDFLAGS=$(BASE_LDFLAGS) $(DEBUG_LDFLAGS)
debug: clean $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf build/ $(BIN)
