# - - - - - - - - - - - - - - - - - - - - - - - -
#
# Makefile pro NESinC
# Compila a linguagem
# 
# Uso (rode pelo CMD no mesmo diretório do Makefile):
# make -f Makefile [all|fresh|clean|reformat|document]
#
# Alvos:
# - all: Compila tudo;
# - fresh: Limpa os executáveis, objetos, reformata, documenta e compila.
#          Basicamente compila do zero;
# - clean: Limpa os executáveis e objetos. CUIDADO! Pode apagar coisar
#          indesejadas se usado de forma incorreta;
# - reformat: Reformata o código com clang-format. clang-format precisa
#             estar no seu PATH;
#
# - - - - - - - - - - - - - - - - - - - - - - - -

# -- Pré-compilação --

# Compilador
CC := gcc
LD := gcc

CFLAGS := -Wall -Wextra -pedantic -g3

# Caminhos
BASE := $(CURDIR)

SRC := $(BASE)/src
INC := $(BASE)/inc
OUT := $(BASE)/out
OBJ := $(BASE)/obj
DOC := $(BASE)/doc

CFLAGS += -I$(INC)
CFLAGS += -IC:/SDL2/include
LDFLAGS := -LC:/SDL2/lib -lmingw32 -lSDL2main -lSDL2

# Códigos-fonte
SRCS := $(wildcard $(SRC)/*.c )
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

# -- Main --

.PHONY: all clean reformat fresh

# Compila
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
    
# Clean
# rm -rf basicamente
clean:
	$(RM) $(OUT)/*.exe
	$(RM) $(OBJ)/*.o
	clear

# Reformat
# Reformata o código usando clang-format
reformat:
	clang-format $(SRC)/*.c -style=file -i
	clang-format $(INC)/*.h -style=file -i

# Fresh
# Limpa os objetos/exe, reformata o código e compila de novo
fresh: clean reformat all

