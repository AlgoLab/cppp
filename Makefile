# Makefile for Constrained Persistent Perfect Phylogeny
SRC_DIR	=src
OBJ_DIR	=obj
LIB_DIR =lib
INTERNAL_TEST_DIR = tests/internal
API_TEST_DIR = tests/public
BIN_DIR	=bin
CC = gcc

P = $(BIN_DIR)/cppp
SRCS := $(wildcard $(SRC_DIR)/*.c)
SOURCES := $(SRCS:$(SRC_DIR)/%=%)

CFLAGS_STD = -g -Wall -DDEBUG -O0 -march=native -Wno-deprecated -Wno-parentheses -Wno-format
STD_LIBS = glib-2.0 igraph jansson bdw-gc
DEBUG_LIBS = check #efence

LIBS 	= $(LIB_DIR)/getopt/cmdline.o
CFLAGS_EXTRA =  -m64 -std=c11 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
CFLAGS_LIBS = `pkg-config --cflags $(STD_LIBS)`
CFLAGS_TEST =  -DTEST_EVERYTHING  `pkg-config --cflags $(DEBUG_LIBS)`
LDLIBS = `pkg-config --libs $(STD_LIBS)`
LDLIBS_TEST = `pkg-config --libs $(DEBUG_LIBS)`
CFLAGS = $(CFLAGS_STD) $(CFLAGS_EXTRA) $(CFLAGS_LIB)
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o) $(LIBS)
T_SOURCES = $(wildcard $(API_TEST_DIR)/*.c)
T_OBJECTS = $(SOURCES:%.c=$(INTERNAL_TEST_DIR)/%.o)  $(T_SOURCES:%.c=%.o)
CC_FULL = $(CC) $(CFLAGS) -I$(SRC_DIR) -I$(LIB_DIR) $(CFLAGS_LIBS)

bin: $(P)

$(P): $(OBJECTS)
	@echo 'Linking $@'
	@mkdir -p $(BIN_DIR)
	$(CC_FULL) -o $@ $^ $(LDLIBS)

all: $(P) doc check
	echo $(OBJECTS)

${OBJ_DIR}/%.o: $(SRC_DIR)/%.c $(LIBS)
	@echo '* Compiling $<'
	@mkdir -p $(dir $@)
	$(CC_FULL) -MM -MF $(patsubst %.c,%.d,$<)  -MT $@ $<
	$(CC_FULL) -o $@ -c $<

${LIB_DIR}/%.o: $(LIB_DIR)/%.c
	@echo '* Compiling $<'
	@mkdir -p $(dir $@)
	$(CC_FULL) -MM -MF $(patsubst %.c,%.d,$<)  -MT $@ $<
	$(CC_FULL) -o $@ -c $<


${INTERNAL_TEST_DIR}/%.o: $(SRC_DIR)/%.c ${OBJ_DIR}/%.o
	@echo '* Compiling for test $<'
	@mkdir -p $(dir $@)
# tail -n +2 $(patsubst %.c,%.d,$<) | grep -oP "\bsrc\/[^\s]+" | tr ' ' '\n' | perl -e 's/\.[ch]$$/.o/' -p | perl -e 's/^src/obj/' -p | uniq | tr '\n' ' '
# analyzes the dependencies file to extract the object files (*.o) that must be included
#
# It is necessary to generate two different executables (with and without tests) from the same source file
	$(CC_FULL) $(CFLAGS_TEST) -o $@ $< `tail -n +2 $(patsubst %.c,%.d,$<) | grep -oP "\bsrc\/[^\s]+" | tr ' ' '\n' | perl -e 's/\.[ch]$$/.o/' -p | perl -e 's/^src/obj/' -p | uniq | tr '\n' ' '` $(LIBS)$  $(LDLIBS) $(LDLIBS_TEST)

${API_TEST_DIR}/%.o: $(API_TEST_DIR)/%.c
	@echo '* Compiling for test $<'
	@mkdir -p $(dir $@)
	$(CC_FULL) -MM -MF $(patsubst %.c,%.d,$<)  -MT $@ $<
	$(CC_FULL) $(CFLAGS_TEST) -o $@ $< `tail -n +2 $(patsubst %.c,%.d,$<) | grep -oP "\bsrc\/[^\s]+" | tr ' ' '\n' | perl -e 's/\.[ch]$$/.o/' -p | perl -e 's/^src/obj/' -p | uniq | tr '\n' ' '` $(LIBS)$  $(LDLIBS) $(LDLIBS_TEST)

clean:
	@echo "Cleaning..."
	rm -rf ${TEST_DIR}/*.o ${OBJ_DIR} ${BIN_DIR} $(SRC_DIR)/*.d $(LIB_DIR)/getopt

check: $(T_OBJECTS) bin
	tests/internal/perfect_phylogeny.o

doc: $(P) docs/latex/refman.pdf
	doxygen && cd docs/latex/ && latexmk -recorder -use-make -pdf refman

.PHONY: all clean doc

ifneq "$(MAKECMDGOALS)" "clean"
-include ${SOURCES:.c=.d}
-include ${T_SOURCES:.c=.d}
endif

$(LIB_DIR)/getopt/cmdline.c $(LIB_DIR)/getopt/cmdline.h: cppp.ggo
	@mkdir -p $(LIB_DIR)/getopt
	gengetopt -i $< --output-dir=$(LIB_DIR)/getopt
