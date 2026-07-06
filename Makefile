CC		:= gcc
CFLAGS 	:= -Wall -Wextra -pedantic -std=c11 -Iinclude
LDGLAGS	:= -lncurses -lpthread

SRC_DIR	:= src
OBJ_DIR	:= build/obj
BIN		:= NetDet

SRCS	:= $(wildcard $(SRC_DIR)/*.c)
OBJS	:= $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean debug

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

debug: CFLAGS += -g -O0 -DDEBUG
debug: all

clean:
	rm -rf build/ $(BIN)
