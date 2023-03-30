// -*- compile-command: "cd .. && make test" -*-
#pragma once

#include <cassert>
#include <iterator>

namespace avm::fault_injection
{
	struct point_t
	{
		const char * const space;
		const char * const name;
		int error_code;
		bool active;
	};

#define FAULT_INJECTION_POINT_REF(space, name) ::space::fault_injection_point_##name

#if defined(__APPLE__)
#define FAULT_INJECTION_POINT(space, name)	  \
	namespace space { \
		::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used)) = { #space, #name, 0, false }; \
		static ::avm::fault_injection::point_t * fault_injection_point_##name##_ptr __attribute__((used,section("__DATA,__faults"))) = &FAULT_INJECTION_POINT_REF(space, name); \
	}
#elif defined(__linux__)
#define FAULT_INJECTION_POINT(space, name)                              \
	namespace space { \
		::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used)) = { #space, #name, 0, false }; \
		static ::avm::fault_injection::point_t * fault_injection_point_##name##_ptr __attribute__((used,section("__faults"))) = &FAULT_INJECTION_POINT_REF(space, name); \
	}
#else
#error "Unsupported platform"
#endif

#define FAULT_INJECT_ERROR_CODE(space, name, action) (FAULT_INJECTION_POINT_REF(space, name).active ? FAULT_INJECTION_POINT_REF(space, name).error_code : (action))
#define FAULT_INJECT_ERRNO_EX(space, name, action, result) (FAULT_INJECTION_POINT_REF(space, name).active ? ((errno = FAULT_INJECTION_POINT_REF(space, name).error_code), (result)) : (action))
#define FAULT_INJECT_ERRNO(space, name, action) FAULT_INJECT_ERRNO_EX(space, name, action, -1)
#define FAULT_INJECT_EXCEPTION(space, name, exception) do { if (FAULT_INJECTION_POINT_REF(space, name).active) { throw (exception); } } while (false)

	point_t * find(const char * space, const char * name);

	bool isActive(const char * space, const char * name);

	inline bool isActive(const point_t & point)
	{
		return point.active;
	}

	void activate(const char * space, const char * name, bool active = true);

	inline void activate(point_t & point, bool active = true)
	{
		point.active = active;
	}

	void setErrorCode(const char * space, const char * name, int error = 0);

	inline void setErrorCode(point_t & point, int error = 0)
	{
		point.error_code = error;
	}

	class points_collection
	{
	public:
		class iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = point_t;
			using pointer           = point_t *;
			using reference         = point_t &;

			iterator(point_t ** ptr):
				ptr_(ptr)
			{}

			reference operator *() const noexcept
			{
				assert(ptr_ != nullptr);

				return **ptr_;
			}

			pointer operator ->() const noexcept
			{
				assert(ptr_ != nullptr);

				return *ptr_;
			}

			iterator& operator ++()
			{
				++ptr_;

				return *this;
			}

			iterator operator ++(int)
			{
				iterator result{*this};

				++ptr_;

				return result;
			}

			bool operator ==(const iterator& rhs) const
			{
				return ptr_ == rhs.ptr_;
			}

			bool operator !=(const iterator& rhs) const
			{
				return ptr_ != rhs.ptr_;
			}

		private:
			point_t ** ptr_;
		};

		iterator begin();
		iterator end();
	};

	extern points_collection points;
}
