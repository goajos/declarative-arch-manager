CC = gcc
CFLAGS = -g -std=c23 -Wall -Wextra -Werror -pedantic
CFLAGS += -I$(CKDL_DIR)/include
LDFLAGS = -lc -lm
LDFLAGS += -L$(CKDL_BUILD_DIR)
LDFLAGS += -lkdl

BIN = bin/damgr
BIN_DIR = /usr/bin

BUILD_DIR = build
SRC = main.c \
	  src/commands/utils.c \
	  src/state/utils.c \
	  src/state/action.c \
	  src/state/config.c \
	  src/state/host.c \
	  src/state/module.c
OBJ = $(SRC:%.c=$(BUILD_DIR)/%.o)

CKDL_DIR = libs/ckdl
CKDL_BUILD_DIR = $(CKDL_DIR)/build
CKDL_LIB = $(CKDL_BUILD_DIR)/libkdl.a
$(CKDL_LIB):
	mkdir -p $(CKDL_BUILD_DIR)
	cd $(CKDL_BUILD_DIR) && cmake ..
	$(MAKE) -C $(CKDL_BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ) $(CKDL_LIB) # bin depends on ckdl_lib
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(CKDL_BUILD_DIR)

install: $(BIN)
	install -m 755 $(BIN) $(BIN_DIR)/damgr
	mkdir -p /usr/share/damgr
	cp -r damgr/* /usr/share/damgr

uninstall:
	rm -f $(BIN_DIR)/damgr
	rm -rf /usr/share/damgr # clear share data
