// -*- compile-command: "cd .. && make test" -*-
#include <fault_injection.hpp>

#include <string.h>

#include <algorithm>

#if defined(__APPLE__)
__attribute__((visibility("hidden")))
extern avm::fault_injection::point_t * first_injection __asm("section$start$__DATA$__faults");
__attribute__((visibility("hidden")))
extern avm::fault_injection::point_t * last_injection  __asm("section$end$__DATA$__faults");
__attribute__((visibility("hidden")))
static avm::fault_injection::detail::module_points_t fault_injections = {
	.next = nullptr,
	.begin = &first_injection,
	.end = &last_injection
};
#elif defined(__linux__)
__attribute__((visibility("hidden")))
extern avm::fault_injection::point_t *__start___faults;
__attribute__((visibility("hidden")))
extern avm::fault_injection::point_t *__stop___faults;
static avm::fault_injection::detail::module_points_t fault_injections = {
	.next = nullptr,
	.begin = &__start___faults,
	.end = &__stop__faults
};
#else
#error "Unsupported platform"
#endif

avm::fault_injection::points_collection avm::fault_injection::points{};

namespace avm::fault_injection
{
	__attribute__((weak))
	detail::module_points_t * getModule()
	{
		return &fault_injections;
	}

	__attribute__((weak))
	void registerModuleImpl(detail::module_points_t * points)
	{
		if (points->next != nullptr) {
			return;
		}
		if (getModule() == points) {
			return;
		}

		detail::module_points_t * module = getModule();

		do {
			if (module->begin == points->begin) {
				return;
			}
		} while ((module->next != nullptr) && (module = module->next));

		module->next = points;
		points->next = nullptr;
	}

	__attribute__((weak))
	void unregisterModule(detail::module_points_t * /*points*/)
	{
	}
}

avm::fault_injection::point_t * avm::fault_injection::find(const char * space, const char * name)
{
	auto item = std::find_if(points.begin(), points.end(), [space, name](point_t & point) {
		return (strcmp(point.space, space) == 0) && (strcmp(point.name, name) == 0);
	});

	if (item != points.end()) {
		return &*item;
	}

	return nullptr;
}

bool avm::fault_injection::isActive(const char * space, const char * name)
{
	point_t * point = find(space, name);

	return (point != nullptr) ? point->active : false;
}

bool avm::fault_injection::isActive(const point_t & point)
{
	return point.active;
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
	return iterator{getModule()};
}

avm::fault_injection::points_collection::iterator avm::fault_injection::points_collection::end()
{
	return iterator{};
}

void avm::fault_injection::registerModule()
{
	avm::fault_injection::registerModuleImpl(&fault_injections);
}

__attribute__((used,constructor))
static void init()
{
	avm::fault_injection::registerModule();
}

/*__attribute__((used,destructor))
static void deinit()
{
	std::cout << "Deinit\n";
	avm::fault_injection::unregisterModule(&fault_injections);
	}*/
