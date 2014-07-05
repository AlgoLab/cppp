# Makefile for Constrained Persistent Perfect Phylogeny

SRC_DIR	=src/
OBJ_DIR	=obj/
BIN_DIR	=bin/

CFLAGS	= -g -Wall -DDEBUG -O2 -march=native -Wno-deprecated
STD_LIBS = glib-2.0 igraph
LIBS 	= lib/igraph/igraph
CFLAGS_EXTRA =  -m64 -std=c1 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -g
CFLAGS_DEBUG = # -lefence
CFLAGS_LIBS = `pkg-config --cflags $(STD_LIBS)`
LDLIBS = `pkg-config --libs $(STD_LIBS)`

all: cppp


${OBJ_DIR}%.o: $(SRC_DIR)%.c
	@echo '* Compiling $<'
	@mkdir -p $(dir $@)
	gcc  $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_DEBUG)   \
	-o $@ \
	-c $< -I$(SRC_DIR) $(CFLAGS_LIBS)


cppp: $(OBJ_DIR)cppp.o
	@echo 'Linking $@'
	@mkdir -p $(BIN_DIR)
	gcc  $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_DEBUG)  \
	-o $(BIN_DIR)$@ $^ -I$(SRC_DIR) $(LDLIBS)


clean:
	@echo "Cleaning..."
	rm -rf ${OBJ_DIR} ${BIN_DIR}
