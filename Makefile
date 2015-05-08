# Makefile for Constrained Persistent Perfect Phylogeny
SRC_DIR	=src
OBJ_DIR	=obj
LIB_DIR =lib
TEST_DIR = tests
BIN_DIR	=bin
CC = gcc

P = $(BIN_DIR)/cppp
SRCS := $(wildcard $(SRC_DIR)/*.c)
SOURCES := $(SRCS:$(SRC_DIR)/%=%)

CFLAGS_STD = -g -Wall -march=native -Wno-deprecated -Wno-parentheses -Wno-format
STD_LIBS = glib-2.0 bdw-gc
DEBUG_LIBS = #efence

LIBS 	= $(LIB_DIR)/getopt/cmdline.o
CFLAGS_EXTRA =  -m64 -std=c11 -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
CFLAGS_LIBS = `pkg-config --cflags $(STD_LIBS)`
LDLIBS = `pkg-config --libs $(STD_LIBS)`
CFLAGS = $(CFLAGS_STD) $(CFLAGS_EXTRA) $(CFLAGS_LIB)
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o) $(LIBS)
CC_FULL = $(CC) $(CFLAGS) -I$(SRC_DIR) -I$(LIB_DIR) $(CFLAGS_LIBS)

dist: CFLAGS +=  -O3
dist: clean bin
bin: $(P)

debug: CFLAGS += -DDEBUG -O0

debug: bin

profile: CFLAGS += -O3 -pg

profile: bin

$(P): $(OBJECTS)
	@echo 'Linking $@'
	@mkdir -p $(BIN_DIR)
	$(CC_FULL) -o $@ $^ $(LDLIBS)

all: $(P) doc
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


clean: clean-test
	@echo "Cleaning..."
	rm -rf  ${OBJ_DIR} ${BIN_DIR} $(SRC_DIR)/*.d $(LIB_DIR)/getopt cmdline.[ch]

clean-test:
	@echo "Cleaning tests..."
	rm -rf ${TEST_DIR}/*.o ${TEST_DIR}/*/output/*

# The regression tests directory structure is:
# tests/regression/input    : input matrix
# tests/regression/output   : actual outputs and diffs
# tests/regression/ok       : expected outputs
REG_TESTS_DIR := tests/regression
REG_TESTS_OK   := $(wildcard $(REG_TESTS_DIR)/ok/*)
REG_TESTS_DIFF := $(REG_TESTS_OK:$(REG_TESTS_DIR)/ok/%=$(REG_TESTS_DIR)/output/%.diff)

test: tests
test: dist $(REG_TESTS_OK)
	tests/bin/run-tests.sh


doc: dist docs/latex/refman.pdf
	doxygen && cd docs/latex/ && latexmk -recorder -use-make -pdf refman

.PHONY: all clean doc unit-test clean-test regression-test

ifneq "$(MAKECMDGOALS)" "clean"
-include ${SOURCES:.c=.d}
-include ${T_SOURCES:.c=.d}
endif

$(LIB_DIR)/getopt/cmdline.c $(LIB_DIR)/getopt/cmdline.h: cppp.ggo
	@mkdir -p $(LIB_DIR)/getopt
	gengetopt -i $< --output-dir=$(LIB_DIR)/getopt
