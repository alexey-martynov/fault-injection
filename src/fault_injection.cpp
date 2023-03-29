// -*- compile-command: "cd .. && make test" -*-
#include <fault_injection.hpp>

#include <string.h>

#if defined(__APPLE__)
extern avm::fault_injection::point_t start_injections __asm("section$start$__DATA$__faults");
extern avm::fault_injection::point_t stop_injections  __asm("section$end$__DATA$__faults");
#else
#error "Unsupported platform"
#endif

avm::fault_injection::points_collection avm::fault_injection::points{};

avm::fault_injection::point_t * avm::fault_injection::find(const char * space, const char * name)
{
	for (point_t * pt = &start_injections, * const end = &stop_injections; pt != end; ++pt) {
		if ((strcmp(pt->space, space) == 0) && (strcmp(pt->name, name) == 0)) {
			return pt;
		}
	}

	return nullptr;
}

bool avm::fault_injection::isActive(const char * space, const char * name)
{
	point_t * point = find(space, name);

	return (point != nullptr) ? point->active : false;
}

void avm::fault_injection::activate(const char * space, const char * name, bool active)
{
	point_t * point = find(space, name);

	if (point != nullptr) {
		point->active = active;
	}
}

void avm::fault_injection::setErrorCode(const char * space, const char * name, int error)
{
	point_t * point = find(space, name);

	if (point != nullptr) {
		point->error_code = error;
	}
}

avm::fault_injection::points_collection::iterator avm::fault_injection::points_collection::begin()
{
	return iterator{&start_injections};
}

avm::fault_injection::points_collection::iterator avm::fault_injection::points_collection::end()
{
	return iterator{&stop_injections};
}
