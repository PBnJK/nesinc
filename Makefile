# - - - - - - - - - - - - - - - - - - - - - - - -
#
# Makefile pro NESinC
# Compiles the emulator
#
# Use:
# ~ make -f Makefile [all|fresh|clean|reformat|document]
#
# Targets:
# - all: Compiles all;
# - fresh: Runs clean, reformat and all, running a fresh compilation;
# - clean: rm -rf's .o and .exe files;
# - reformat: Reformats the codebase with clang-format. Make sure it's on
#             your path :)
#
# - - - - - - - - - - - - - - - - - - - - - - - -

# -- Pre-compilation step --

# Compiler + linker
CC := gcc
LD := gcc

SANITIZE := -fsanitize=undefined -fsanitize-trap

# Flags from: https://nullprogram.com/blog/2023/04/29
CFLAGS := -g3 -Wall -Wextra -Wconversion -Wdouble-promotion

# Paths
BASE := $(CURDIR)

SRC := $(BASE)/src
INC := $(BASE)/inc
OUT := $(BASE)/out
OBJ := $(BASE)/obj
DOC := $(BASE)/doc

CFLAGS += -I$(INC)
CFLAGS += -IC:/SDL2/include

CFLAGS += $(SANITIZE)
LDFLAGS := -LC:/SDL2/lib -lmingw32 -lSDL2main -lSDL2 $(SANITIZE)

# Source files
SRCS := $(wildcard $(SRC)/*.c )
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

# -- Main --

.PHONY: all clean reformat fresh

all: $(OBJS)
	@echo
	@echo Linking $@
	@echo ...
	@echo
	$(LD) $(OBJS) $(LDFLAGS) -o $(OUT)/nesinc.exe
	@echo
	@echo All done!

$(OBJ)/%.o: $(SRC)/%.c
	@echo Compiling $@
	@echo ...
	@echo
	$(CC) -c $(CFLAGS) $^ -o $@
	@echo
	@echo Done
	@echo
    

clean:
	$(RM) $(OUT)/*.exe
	$(RM) $(OBJ)/*.o
	clear

reformat:
	clang-format $(SRC)/*.c -style=file -i
	clang-format $(INC)/*.h -style=file -i

fresh: clean reformat all

