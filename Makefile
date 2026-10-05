# Colour-ball detector (Projet Fil Rouge, part 1).
#
#   make            build the detector (PFR, or PFR.exe on Windows)
#   make test       unit tests + regression tests on the 20 photos
#   make asan       same tests with AddressSanitizer and UBSan (Linux, macOS)
#   make clean      remove everything that was built
#
# On Windows with MinGW, run mingw32-make from Git Bash or MSYS.

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
SANITIZE = -std=c11 -Wall -Wextra -pedantic -O1 -g -fno-omit-frame-pointer \
           -fsanitize=address,undefined -fno-sanitize-recover=all

ifeq ($(OS),Windows_NT)
EXE = .exe
endif

TARGET = PFR$(EXE)
BUILD = build

LIB_SRCS = file_operations.c image_process.c cluster.c
SRCS = main.c $(LIB_SRCS)
HEADERS = file_operations.h image_process.h cluster.h

GEN_IMAGE = $(BUILD)/gen_image$(EXE)
TEST_UNITS = $(BUILD)/test_units$(EXE)

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

$(BUILD):
	mkdir -p $(BUILD)

$(GEN_IMAGE): tests/gen_image.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ tests/gen_image.c

$(TEST_UNITS): tests/test_units.c $(LIB_SRCS) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -I. -o $@ tests/test_units.c $(LIB_SRCS)

test: $(TARGET) $(GEN_IMAGE) $(TEST_UNITS)
	$(TEST_UNITS) 2> $(BUILD)/test_units.log
	sh tests/run_tests.sh ./$(TARGET) $(GEN_IMAGE)

asan: $(GEN_IMAGE)
	$(CC) $(SANITIZE) -o $(BUILD)/PFR_asan$(EXE) $(SRCS)
	$(CC) $(SANITIZE) -I. -o $(BUILD)/test_units_asan$(EXE) tests/test_units.c $(LIB_SRCS)
	$(BUILD)/test_units_asan$(EXE) 2> $(BUILD)/test_units_asan.log || { cat $(BUILD)/test_units_asan.log; exit 1; }
	sh tests/run_tests.sh $(BUILD)/PFR_asan$(EXE) $(GEN_IMAGE)

clean:
	rm -rf $(BUILD) $(TARGET)

.PHONY: all test asan clean
