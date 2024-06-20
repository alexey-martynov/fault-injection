// -*- compile-command: "cd .. && make test" -*-
#include <fault_injection.hpp>

#include <iostream>

FAULT_INJECTION_POINT(lib, point1, "Library Point 1");

// Version 0 point
#if defined(__APPLE__)
namespace lib {
	::avm::fault_injection::v1::point_t fault_injection_point_point_v0 __attribute__((used)) = { "lib", "point_v0", "Point version 0", 0, false, ::avm::fault_injection::mode_t::multiple };
	static ::avm::fault_injection::point_t * fault_injection_point_point_v0_ptr __attribute__((used,section("__DATA,__faults"))) = reinterpret_cast<::avm::fault_injection::point_t *>(&fault_injection_point_point_v0);
}
#elif defined(__linux__)
namespace lib {
	::avm::fault_injection::point_t fault_injection_point_point_v0 __attribute__((used)) = { "lib", "point_v0", "Point version 0", 0, false, ::avm::fault_injection::mode_t::multiple };
	static ::avm::fault_injection::point_t * fault_injection_point_point_v0_ptr __attribute__((used,section("__faults"))) = reinterpret_cast<::avm::fault_injection::point_t *>(&fault_injection_point_point_v0);
}
#else
#error "Unsupported platform"
#endif

void executeWithInjection()
{
	FAULT_INJECT_EXCEPTION(lib, point1, std::runtime_error("INJECTED"));
}

void executeV0WithInjection()
{
	// Code manually
	avm::fault_injection::registerModule();

	if (FAULT_INJECTION_READ(lib::fault_injection_point_point_v0.active)) {
		if (FAULT_INJECTION_READ(lib::fault_injection_point_point_v0.mode) == ::avm::fault_injection::mode_t::multiple) {
			FAULT_INJECTION_WRITE(lib::fault_injection_point_point_v0.active, false);
		}

		throw std::runtime_error("INJECTED");
	}
}
