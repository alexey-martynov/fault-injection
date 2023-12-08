.PHONY: all clean test install

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
INSTALL      := install
libdir       ?= lib64

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

install: libavm_fault_injection.a include/fault_injection.hpp include/fault_injection_test_helper.hpp
	@test "$(DESTDIR)" || (echo "No DESTDIR specified. Installation is not possible." >&2 ; exit 1)
	$(INSTALL) -m 755 -d "$(DESTDIR)/include"
	$(INSTALL) -m 644 -p include/fault_injection.hpp "$(DESTDIR)/include"
	$(INSTALL) -m 644 -p include/fault_injection_test_helper.hpp "$(DESTDIR)/include"
	$(INSTALL) -m 755 -d "$(DESTDIR)/$(libdir)"
	$(INSTALL) -m 644 -p libavm_fault_injection.a "$(DESTDIR)/$(libdir)"

ifneq 'clean' '$(findstring clean,$(MAKECMDGOALS))'
include $(wildcard src/*.d) $(wildcard test/*.d)
endif

