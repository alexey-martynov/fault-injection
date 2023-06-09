.PHONY: all clean test

platform     := $(shell uname)
ifeq '$(platform)' 'Darwin'
shared_lib_suffix := dylib
shared_switch     := -dynamiclib
else
shared_lib_suffix := so
shared_switch     := -shared
endif

CC           := $(CXX)
CXX_STANDARD  = c++17
CXXFLAGS     += -std=$(CXX_STANDARD) -Iinclude -fPIC -g -Wall -Wextra -Werror -MD -MF $(@:.o=.d)
LDFLAGS      += -g
LDLIBS       := -lboost_unit_test_framework

all: libavm_fault_injection.a

libavm_fault_injection.a: src/fault_injection.o
	ar rcs $@ $^

test/test: test/test.o libavm_fault_injection.a

test/test-shared: test/test-shared.o test/libtest.$(shared_lib_suffix) libavm_fault_injection.a

test/test-disabled-shared: test/test-disabled-shared.o test/libtest.$(shared_lib_suffix) libavm_fault_injection.a

test/libtest.$(shared_lib_suffix): test/libtest.o libavm_fault_injection.a
	$(CXX) -o $@ $(LDFLAGS) $(shared_switch) $^

test/test.o test/libtest.o test/test-shared.o: %.o: %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) -DFAULT_INJECTIONS_ENABLED=1 $<

test/test-disabled-shared.o: %.o: %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) -DFAULT_INJECTIONS_ENABLED=0 $<

test: test/test test/test-shared test/test-disabled-shared
	test/test
	test/test-shared
	test/test-disabled-shared

clean:
	rm -f libavm_fault_injection.a $(wildcard src/*.o) $(wildcard src/*.d) test/test test/test-shared $(wildcard test/*.$(shared_lib_suffix)) $(wildcard test/*.o) $(wildcard test/*.d)

ifneq 'clean' '$(findstring clean,$(MAKECMDGOALS))'
include $(wildcard src/*.d) $(wildcard test/*.d)
endif

