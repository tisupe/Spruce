CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Isrc

BUILD_DIR = build
TARGET = $(BUILD_DIR)/needle
SRC = src/main.cpp
HEADERS = src/handler.hpp src/inbuilt.hpp

all: $(TARGET)
$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run:
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)/*
