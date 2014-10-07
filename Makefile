# Makefile for Constrained Persistent Perfect Phylogeny
SRC_DIR	=src
OBJ_DIR	=obj
LIB_DIR =lib
TEST_DIR = tests
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
CFLAGS_EXTRA =  -m64 -std=c11 -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
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

clean: clean-test
	@echo "Cleaning..."
	rm -rf  ${OBJ_DIR} ${BIN_DIR} $(SRC_DIR)/*.d $(LIB_DIR)/getopt

clean-test:
	@echo "Cleaning tests..."
	rm -rf ${TEST_DIR}/*.o ${TEST_DIR}/*/output/*

# The regression tests are stored in a directory tree as follows:
# tests/
# tests/TYPE                : TYPE is api for perfect_phylogeny tests. Contains the description of each test
# tests/TYPE/input          : contains the input files. They can be instances or graphs
# tests/TYPE/output         : the outputs of the execution of the tests, and diffs wrt expected results
# tests/TYPE/ok             : the expected results
#
# The filenames follow this scheme:
# test description name: t*.json
# actual results and expected results: *.json
# The stem of the filenames (the expansion of *) must be the same for all tests
#
PP_TESTS_DIR := tests/api
PP_TESTS_OK   := $(wildcard $(PP_TESTS_DIR)/ok/*)
# PP_TESTS_OK_T stores a result for each test description
# It checks that I never forget to write the expected results
# PP_TESTS_OK_T doesn't contain the graph files
PP_TESTS_OK_T := $(PP_TESTS_OK:$(PP_TESTS_DIR)/t%.json=$(PP_TESTS_DIR)/ok/%.json)
PP_TESTS_OUT  := $(PP_TESTS_OK:$(PP_TESTS_DIR)/ok/%=$(PP_TESTS_DIR)/output/%)
PP_TESTS_DIFF := $(PP_TESTS_OK:$(PP_TESTS_DIR)/ok/%=$(PP_TESTS_DIR)/output/%.diff)

check: bin $(T_OBJECTS) unit-test api-test

tests: check regression-test

# Implicit rules to perform the API tests
$(PP_TESTS_DIR)/output/%.json $(PP_TESTS_DIR)/output/%.json-conflict.graphml $(PP_TESTS_DIR)/output/%.json-redblack.graphml: $(PP_TESTS_DIR)/t%.json $(T_OBJECTS)
	tests/internal/perfect_phylogeny.o $<

# We must ignore diff's return value, otherwise make complains
$(PP_TESTS_DIR)/output/%.diff: $(PP_TESTS_DIR)/output/%
	-diff -u --strip-trailing-cr --ignore-all-space $< $(PP_TESTS_DIR)/ok/$* > $@

api-test: $(PP_TESTS_DIFF) $(PP_TESTS_OK_T) $(PP_TESTS_OUT)
	cat $(PP_TESTS_DIFF)

unit-test: $(T_OBJECTS) bin
	tests/internal/perfect_phylogeny.o

# The regression tests directory structure is:
# tests/regression/input    : input matrix
# tests/regression/output   : actual outputs and diffs
# tests/regression/ok       : expected outputs
REG_TESTS_DIR := tests/regression
REG_TESTS_OK   := $(wildcard $(REG_TESTS_DIR)/ok/*)
REG_TESTS_DIFF := $(REG_TESTS_OK:$(REG_TESTS_DIR)/ok/%=$(REG_TESTS_DIR)/output/%.diff)

regression-test: $(P) $(REG_TESTS_OK)
	tests/bin/run-tests.sh


doc: $(P) docs/latex/refman.pdf
	doxygen && cd docs/latex/ && latexmk -recorder -use-make -pdf refman

.PHONY: all clean doc unit-test clean-test regression-test api-test

ifneq "$(MAKECMDGOALS)" "clean"
-include ${SOURCES:.c=.d}
-include ${T_SOURCES:.c=.d}
endif

$(LIB_DIR)/getopt/cmdline.c $(LIB_DIR)/getopt/cmdline.h: cppp.ggo
	@mkdir -p $(LIB_DIR)/getopt
	gengetopt -i $< --output-dir=$(LIB_DIR)/getopt
