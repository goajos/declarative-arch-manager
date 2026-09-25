CC=gcc
BIN=bin/damgr
# code can be found in . and lib
CODEDIRS=. lib
# include can be found in include
INCDIRS=include
# generate files that encode make rules for the user header deps (-MD includes system headers)
DEPFLAGS=-MMD
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c23 -g $(foreach D,$(INCDIRS),-I$(D)) $(DEPFLAGS)

BUILDDIR=build
CFILES=main.c \
			 src/utils/log.c \
			 src/utils/utils.c \
			 src/state/state.c \
			 src/tasks/tasks.c
OBJECTS=$(patsubst %.c,$(BUILDDIR)/%.o,$(CFILES))
DEPFILES=$(patsubst %.c,$(BUILDDIR)/%.d,$(CFILES))

all: $(BIN)

$(BIN): $(OBJECTS)
	@mkdir -p -m 0755 bin # @<cmd> runs silently
	$(CC) -lm -o $@ $^

# $@ pastes %.o (target)
# $^ pastes %.c (source)
# => how to make target from source
$(BUILDDIR)/%.o:%.c
	@mkdir -p -m 0755 $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BIN) $(BUILDDIR)

-include $(DEPFILES)
