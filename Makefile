# Makefile for Constrained Persistent Perfect Phylogeny
P = cppp
OBJ = cppp.o perfect_phylogeny.o
SRC_DIR	=src/
OBJ_DIR	=obj/
BIN_DIR	=bin/

CFLAGS_STD = -g -Wall -DDEBUG -O2 -march=native -Wno-deprecated
STD_LIBS = glib-2.0 igraph
LIBS 	= lib/igraph/igraph
CFLAGS_EXTRA =  -m64 -std=c11 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -g
CFLAGS_TEST =  -DTEST_EVERYTHING
CFLAGS_DEBUG = # -lefence
CFLAGS_LIBS = `pkg-config --cflags $(STD_LIBS)`
LDLIBS = `pkg-config --libs $(STD_LIBS)`
CFLAGS = $(CFLAGS_STD) $(CFLAGS_EXTRA) $(CFLAGS_DEBUG) $(CFLAGS_LIB)

all: $(P)

$(P): $(OBJECTS)

${OBJ_DIR}%.o: $(SRC_DIR)%.c
	@echo '* Compiling $<'
	@mkdir -p $(dir $@)
	gcc  $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_DEBUG)   \
	-o $@ \
	-c $< -I$(SRC_DIR) $(CFLAGS_LIBS)


cppp: $(OBJ_DIR)cppp.o $(OBJ_DIR)perfect_phylogeny.o
	@echo 'Linking $@'
	@mkdir -p $(BIN_DIR)
	gcc  $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_DEBUG)  \
	-o $(BIN_DIR)$@ $^ -I$(SRC_DIR) $(LDLIBS)


clean:
	@echo "Cleaning..."
	rm -rf ${OBJ_DIR} ${BIN_DIR}

doc: cppp docs/latex/refman.pdf
	doxygen && cd docs/latex/ && latexmk -recorder -use-make -pdf refman

.PHONY: all clean doc
