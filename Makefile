CC := gcc
CFLAGS := -g -std=c23 -Wall -Wextra -Werror -pedantic
LDFLAGS := -lc -lm

BIN := bin/damngr
BIN_DIR := /usr/bin

BUILD_DIR := build
all:
	mkdir -p build

SRC := main.c \
	  src/commands/utils.c \
	  src/state/utils.c \
	  src/state/action.c \
	  src/state/config.c \
	  src/state/host.c \
	  src/state/module.c

OBJ := $(SRC:%=$(BUILD_DIR)/%.o)

CKDL_DIR := libs/ckdl
CKDL_BUILD_DIR := $(CKDL_DIR)
CKDL_LIB := $(CKDL_BUILD)/libkdl
# CFLAGS += -l$(CKDL_BUILD)/include/kdl.h
# CFLAGS += -l$(CKDL_BUILD)/include/parser.h
# CFLAGS += -l$(CKDL_BUILD)/include/emitter.h
# LDFLAGS += $(CKDL_LIB)
$(CKDL_LIB):
	mkdir -p $(CKDL_BUILD_DIR)
	cd $(CKDL_BUILD_DIR) && cmake ..
	$(MAKE) -C $(CKDL_BUILD_DIR)
LDFLAGS += -lkdl

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

# $(BUILD_DIR)%.c.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(CKDL_BUILD_DIR)
.PHONY: clean

install: $(BIN)
	install -m 755 $(BIN) $(BIN_DIR)/damngr

uninstall:
	rm -f $(BIN_DIR)/damngr
