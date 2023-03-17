.PHONY: all clean test

CC           := $(CXX)
CXX_STANDARD  = c++17
CXXFLAGS     += -std=$(CXX_STANDARD) -Iinclude -g -Wall -Wextra -Werror -MD -MF $(@:.o=.d)
LDFLAGS      += -g
LDLIBS       := -lboost_unit_test_framework

all: libavm_fault_injection.a test/test

libavm_fault_injection.a: src/fault_injection.o
	ar rcs $@ $^

test/test: test/test.o libavm_fault_injection.a

test: test/test
	$<

clean:
	rm -f libavm_fault_injection.a $(wildcard src/*.o) $(wildcard src/*.d) test/test $(wildcard test/*.o) $(wildcard test/*.d)

include $(wildcard src/*.d)
