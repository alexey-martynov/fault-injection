#pragma once

namespace avm::fault_injection
{
	struct point_t
	{
		const char * space;
		const char * name;
		int error_code;
		bool active;
	};

#define FAULT_INJECTION_POINT(space, name) \
	namespace ::space { \
		static ::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used,section("__DATA,__faults"))) = { #space, #name, 0, false } \
	} \

#define FAULT_INJECT_ERROR_CODE(space, name, action) (space::fault_injection##name.active ? space::fault_injection##name.error_code : (action))
#define FAULT_INJECT_ERRNO_EX(space, name, action, result) (space::fault_injection##name.active ? ((errno = space::fault_injection##name.error_code), (result)) : (action))
#define FAULT_INJECT_ERRNO(space, name, action) FAULT_INJECT_ERRNO_EX(space, name, action, -1)
#define FAULT_INJECT_EXCEPTION(space, name, exception) do { if (space::fault_injection##name.active) { throw exception; } } while (false)

	point_t * find(const char * name);

	bool isActive(const char * name);

	void activate(const char * name, bool active = true);

	void setErrorCode(const char * name, int error);

}
