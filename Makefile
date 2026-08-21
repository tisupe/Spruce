CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Isrc

PROJECT = Spruce
BUILD_DIR = build
TARGET = $(BUILD_DIR)/$(PROJECT)
SRC = src/main.cpp
HEADERS = $(wildcard src/*.hpp)

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run:
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)/*
