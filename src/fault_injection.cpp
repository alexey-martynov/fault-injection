#include <fault_injection.hpp>

#include <string.h>

#if defined(__APPLE__)
extern avm::fault_injection::point_t start_injections __asm("section$start$__DATA$__faults");
extern avm::fault_injection::point_t stop_injections  __asm("section$end$__DATA$__faults");
#else
#error "Unsupported platform"
#endif

avm::fault_injection::point_t * avm::fault_injection::find(const char * name)
{
	for (long i = 0; i < (&stop_injections - &start_injections); ++ i) {
		if (strcmp((&start_injections)[i].name, name) == 0) {
			return &start_injections + i;
		}
	}

	return nullptr;
}

bool avm::fault_injection::isActive(const char * name)
{
	point_t * point = find(name);

	return (point != nullptr) ? point->active : false;
}

void avm::fault_injection::activate(const char * name, bool active)
{
	point_t * point = find(name);

	if (point != nullptr) {
		point->active = active;
	}
}

void avm::fault_injection::setErrorCode(const char * name, int error)
{
	point_t * point = find(name);

	if (point != nullptr) {
		point->error_code = error;
	}
}
