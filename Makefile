CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude

SRCS    := $(wildcard src/*.cc) $(wildcard test/*.cc) $(wildcard test/*.cpp)
HEADERS := $(wildcard include/*.h) $(wildcard src/*.h)

OUT_DIR := out
BIN     := $(OUT_DIR)/db_test


$(BIN): $(SRCS) $(HEADERS)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f ./$(BIN)

.PHONY: run clean
