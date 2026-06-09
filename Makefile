CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -fsanitize=address -Iinclude -Iutil

LIB_SRCS := $(wildcard src/*.cc) $(wildcard util/*.cc)
HEADERS  := $(wildcard include/*.h) $(wildcard src/*.h) $(wildcard util/*.h)

OUT_DIR   := out
DB_BIN    := $(OUT_DIR)/db_test
ARENA_BIN := $(OUT_DIR)/arena_test

all: $(DB_BIN) $(ARENA_BIN)

$(DB_BIN): test/db_test.cpp $(LIB_SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) test/db_test.cpp $(LIB_SRCS) -o $(DB_BIN)

$(ARENA_BIN): test/arena_test.cc $(LIB_SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) test/arena_test.cc $(LIB_SRCS) -o $(ARENA_BIN)

run: all
	./$(DB_BIN) && ./$(ARENA_BIN)

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
