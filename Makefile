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
STD_LIBS = bdw-gc
DEBUG_LIBS = #efence

LIBS 	= $(OBJ_DIR)/cmdline.o
CFLAGS_EXTRA =  -m64 -std=c11 -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -fopenmp
CFLAGS_LIBS = `pkg-config --cflags $(STD_LIBS)`
LDLIBS = `pkg-config --libs $(STD_LIBS)`
CFLAGS = $(CFLAGS_STD) $(CFLAGS_EXTRA) $(CFLAGS_LIB)
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o) $(LIBS)
CC_FULL = $(CC) $(CFLAGS) -I$(SRC_DIR) -I$(LIB_DIR) $(CFLAGS_LIBS)

dist: CFLAGS +=  -O3 -DNDEBUG
dist: bin
bin: $(P)

debug: CFLAGS += -DDEBUG -O0
debug: LDLIBS += -lefence
debug: bin

debug-dist: CFLAGS += -DDEBUG -O3

debug-dist: bin

profile: CFLAGS += -O3 -DNDEBUG

profile: dist
	rm -f callgrind.*
	valgrind -q --tool=callgrind --dump-instr=yes $(P) -o /dev/null $(REG_TESTS_DIR)/input/no_8x4.00.txt
	# See the results with kcachegrind


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
	rm -rf  ${OBJ_DIR} ${BIN_DIR} $(SRC_DIR)/*.d $(SRC_DIR)/cmdline.[ch] callgrind.out.*

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

thirdparty/cryptominisat/build/cryptominisat4_simple:
	test -d thirdparty/cryptominisat/build || mkdir thirdparty/cryptominisat/build
	cd thirdparty/cryptominisat/build && cmake .. && make -j4

tests: test 
test: dist $(REG_TESTS_OK) thirdparty/cryptominisat/build/cryptominisat4_simple
	tests/bin/run-tests.sh


doc: dist docs/latex/refman.pdf
	doxygen && cd docs/latex/ && latexmk -recorder -use-make -pdf refman

.PHONY: all clean doc unit-test clean-test regression-test profile

ifneq "$(MAKECMDGOALS)" "clean"
-include ${SOURCES:.c=.d}
-include ${T_SOURCES:.c=.d}
endif

$(SRC_DIR)/cmdline.c $(SRC_DIR)/cmdline.h: cppp.ggo
	gengetopt -i $< --output-dir=$(SRC_DIR)
