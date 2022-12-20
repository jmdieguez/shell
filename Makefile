CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=gnu11
CFLAGS += -Wmissing-prototypes

# To force build a test shell run:
#     make -B -e SHELL_TEST=true
ifdef SHELL_TEST
	CFLAGS += -D SHELL_NO_COLORS -D SHELL_NO_INTERACTIVE
endif

ifdef HISTFILE
	CFLAGS += -D HISTFILE
endif

EXEC := sh
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)

all: $(EXEC)

run: $(EXEC)
	./$(EXEC)

valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(EXEC)

$(EXEC): $(OBJS)

format: .clang-files .clang-format
	xargs -r clang-format -i <$<

clean:
	rm -rf $(EXEC) *.o core vgcore.*

.PHONY: all clean format run valgrind