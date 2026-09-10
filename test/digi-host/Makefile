CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic
DIGI_SRC ?= ../../src/digi_utils.cpp
TARGET := run

.PHONY: test clean

$(TARGET): digi_host_test.cpp $(DIGI_SRC)
	$(CXX) $(CXXFLAGS) -Istubs -o $@ digi_host_test.cpp $(DIGI_SRC)

test: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)
