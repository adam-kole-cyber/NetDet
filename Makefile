CC := gcc
BASE_CFLAGS := -Wall -Wextra -pedantic -std=c11 \
               -D_POSIX_C_SOURCE=200809L \
               -Iinclude

DEBUG_CFLAGS := -g3 -O0 \
                -fsanitize=address,undefined,leak \
                -fno-omit-frame-pointer \
                -DDEBUG

VG_CFLAGS := -g3 -O0 \
             -fno-omit-frame-pointer \
             -DDEBUG

BASE_LDFLAGS := -lpanelw -lncursesw -lpthread -lm
DEBUG_LDFLAGS := -fsanitize=address,undefined,leak

SRC_DIR := src
OBJ_DIR := build/obj
BIN_DIR := build/bin
BIN := NetDet
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TOOL_OUI_DIR      := tools/oui-update
TOOL_OUI_SRC_DIR  := $(TOOL_OUI_DIR)/src
TOOL_OUI_OBJ_DIR  := build/obj-tool-oui
TOOL_OUI_BIN      := $(BIN_DIR)/oui-update
TOOL_OUI_CFLAGS   := $(BASE_CFLAGS) -I$(TOOL_OUI_DIR)/include

TOOL_OUI_SRCS := $(shell find $(TOOL_OUI_SRC_DIR) -type f -name '*.c')
TOOL_OUI_OBJS := $(patsubst $(TOOL_OUI_SRC_DIR)/%.c,$(TOOL_OUI_OBJ_DIR)/%.o,$(TOOL_OUI_SRCS))

.PHONY: all debug valgrind-debug tool-oui clean

all: CFLAGS=$(BASE_CFLAGS)
all: LDFLAGS=$(BASE_LDFLAGS)
all: $(BIN) tool-oui

debug: CFLAGS=$(BASE_CFLAGS) $(DEBUG_CFLAGS)
debug: LDFLAGS=$(BASE_LDFLAGS) $(DEBUG_LDFLAGS)
debug: clean $(BIN) tool-oui

valgrind-debug: CFLAGS=$(BASE_CFLAGS) $(VG_CFLAGS)
valgrind-debug: LDFLAGS=$(BASE_LDFLAGS)
valgrind-debug: clean $(BIN) tool-oui

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

tool-oui: $(TOOL_OUI_BIN)

$(TOOL_OUI_BIN): $(TOOL_OUI_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(TOOL_OUI_OBJS) -o $@ $(LDFLAGS)

$(TOOL_OUI_OBJ_DIR)/%.o: $(TOOL_OUI_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(TOOL_OUI_CFLAGS) $(filter -g3 -O0 -fsanitize=% -fno-omit-frame-pointer -DDEBUG,$(CFLAGS)) -c $< -o $@

clean:
	rm -rf build/ $(BIN) $(TOOL_OUI_BIN)
