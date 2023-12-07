// -*- compile-command: "cd .. && make test" -*-
#include <fault_injection.hpp>

#include <iostream>

FAULT_INJECTION_POINT(lib, point1, "Library Point 1");

void executeWithInjection()
{
	FAULT_INJECT_EXCEPTION(lib, point1, std::runtime_error("INJECTED"));
}
