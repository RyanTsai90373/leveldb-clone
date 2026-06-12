CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -fsanitize=address -Iinclude -Iutil

LIB_SRCS := $(wildcard db/*.cc) $(wildcard util/*.cc)
HEADERS  := $(wildcard include/*.h) $(wildcard db/*.h) $(wildcard util/*.h)

OUT_DIR   := out
DB_BIN    := $(OUT_DIR)/db_test
ARENA_BIN := $(OUT_DIR)/arena_test
INTERNALKEY_BIN := $(OUT_DIR)/internalkey_test
ENCODE_BIN := $(OUT_DIR)/encode_test

all: $(DB_BIN) $(ARENA_BIN) $(INTERNALKEY_BIN) $(ENCODE_BIN)

$(DB_BIN): test/db_test.cc $(LIB_SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) test/db_test.cc $(LIB_SRCS) -o $(DB_BIN)

$(ARENA_BIN): test/arena_test.cc $(LIB_SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) test/arena_test.cc $(LIB_SRCS) -o $(ARENA_BIN)

$(INTERNALKEY_BIN): test/internalkey_test.cc $(LIB_SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) test/internalkey_test.cc $(LIB_SRCS) -o $(INTERNALKEY_BIN)

$(ENCODE_BIN): test/encode_test.cc $(LIB_SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) test/encode_test.cc $(LIB_SRCS) -o $(ENCODE_BIN)

run: all
	./$(DB_BIN) && ./$(ARENA_BIN) && ./$(INTERNALKEY_BIN) && ./$(ENCODE_BIN)

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
