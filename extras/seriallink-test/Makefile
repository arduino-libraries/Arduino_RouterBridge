# Host (PC) unit tests for the seriallink protocol core.
#
#   make -C extras/seriallink-test

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined
SRC_DIR = ../../src/seriallink
SRCS = seriallink_test.cpp $(wildcard $(SRC_DIR)/*.cpp)

all: test

seriallink_test: $(SRCS) $(wildcard $(SRC_DIR)/*.h)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

test: seriallink_test
	./seriallink_test

clean:
	rm -f seriallink_test

.PHONY: all test clean
