CC = gcc
CFLAGS = -g -std=c23 -Wall -Wextra -Werror -pedantic
LDFLAGS = -lc -lm

CKDL_DIR = libs/ckdl
CKDL_BUILD = $(CKDL_DIR)/build
CKDL_LIB = $(CKDL_BUILD)/libkdl.a

CFLAGS += -l$(CKDL_BUILD)/include
LDFLAGS += $(CKDL_LIB)

SRC = main.c \
	  src/commands/utils.c \
	  src/state/utils.c \
	  src/state/action.c \
	  src/state/config.c \
	  src/state/host.c \
	  src/state/module.c

OBJ = $(SRC:.c=.o)

BIN = bin/damngr
BIN_DIR = /usr/bin

all: $(CKDL_LIB) $(BIN)

$(CKDL_LIB):
	mkdir -p $(CKDL_BUILD)
	cd $(CKDL_BUILD) && cmake ..
	$(MAKE) -C $(CKDL_BUILD)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)
	rm -rf $(CKDL_BUILD)

.PHONY: all clean

install: $(BIN)
	install -m 755 $(BIN) $(BIN_DIR)/damngr

uninstall:
	rm -f $(BIN_DIR)/damngr
