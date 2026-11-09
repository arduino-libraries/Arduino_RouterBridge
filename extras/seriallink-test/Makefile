# Host (PC) tests for the seriallink protocol core.
#
#   make -C extras/seriallink-test          # build and run the unit tests
#   make -C extras/seriallink-test peer     # build the interoperability test peer
#
# The peer is used by the interoperability tests of arduino-router, see
# internal/seriallink/interop_test.go there.

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined
SRC_DIR = ../../src/seriallink
CORE_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
CORE_HDRS = $(wildcard $(SRC_DIR)/*.h)

all: test

seriallink_test: seriallink_test.cpp $(CORE_SRCS) $(CORE_HDRS)
	$(CXX) $(CXXFLAGS) -o $@ seriallink_test.cpp $(CORE_SRCS)

seriallink_peer: seriallink_peer.cpp $(CORE_SRCS) $(CORE_HDRS)
	$(CXX) $(CXXFLAGS) -o $@ seriallink_peer.cpp $(CORE_SRCS)

test: seriallink_test
	./seriallink_test

peer: seriallink_peer

clean:
	rm -f seriallink_test seriallink_peer

.PHONY: all test peer clean
