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

	namespace detail
	{
		struct module_points_t
		{
			module_points_t * next;
			avm::fault_injection::point_t ** const begin;
			avm::fault_injection::point_t ** const end;
		};
	}


#if (FAULT_INJECTIONS_ENABLED > 0) || (FAULT_INJECTIONS_DEFINITIONS > 0)

#define FAULT_INJECTION_POINT_REF(space, name) ::space::fault_injection_point_##name

#if defined(__APPLE__)
#define FAULT_INJECTION_POINT_EX(space, name, error_code)	  \
	namespace space { \
		::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used)) = { #space, #name, error_code, false }; \
		static ::avm::fault_injection::point_t * fault_injection_point_##name##_ptr __attribute__((used,section("__DATA,__faults"))) = &FAULT_INJECTION_POINT_REF(space, name); \
	}
#elif defined(__linux__)
#define FAULT_INJECTION_POINT_EX(space, name, error_code)	  \
	namespace space { \
		::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used)) = { #space, #name, error_code, false }; \
		static ::avm::fault_injection::point_t * fault_injection_point_##name##_ptr __attribute__((used,section("__faults"))) = &FAULT_INJECTION_POINT_REF(space, name); \
	}
#else
#error "Unsupported platform"
#endif

#else

#define FAULT_INJECTION_POINT_REF(space, name) nullptr
#define FAULT_INJECTION_POINT_EX(space, name, error_code)

#endif

#define FAULT_INJECTION_POINT(space, name)	FAULT_INJECTION_POINT_EX(space, name, 0)

#if FAULT_INJECTIONS_ENABLED > 0

#define FAULT_INJECT_ERROR_CODE(space, name, action) (::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name)) ? FAULT_INJECTION_POINT_REF(space, name).error_code : (action))
#define FAULT_INJECT_ERRNO_EX(space, name, action, result) (::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name)) ? ((errno = FAULT_INJECTION_POINT_REF(space, name).error_code), (result)) : (action))
#define FAULT_INJECT_EXCEPTION(space, name, exception) do { if (::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name))) { throw (exception); } } while (false)

#else

#define FAULT_INJECT_ERROR_CODE(space, name, action) (action)
#define FAULT_INJECT_ERRNO_EX(space, name, action, result) (action)
#define FAULT_INJECT_EXCEPTION(space, name, exception)

#endif

#define FAULT_INJECT_ERRNO(space, name, action) FAULT_INJECT_ERRNO_EX(space, name, action, -1)

	__attribute__((visibility("hidden")))
	void registerModule();

	__attribute__((visibility("hidden")))
	point_t * find(const char * space, const char * name);

	__attribute__((visibility("hidden")))
	bool isActive(const char * space, const char * name);

	__attribute__((visibility("hidden")))
	bool isActive(const point_t & point);

	__attribute__((visibility("hidden")))
	inline bool isActive(std::nullptr_t)
	{
		return false;
	}

	__attribute__((visibility("hidden")))
	void activate(const char * space, const char * name, bool active = true);

	__attribute__((visibility("hidden")))
	inline void activate(point_t & point, bool active = true)
	{
		point.active = active;
	}

	__attribute__((visibility("hidden")))
	inline void activate(std::nullptr_t, bool = true)
	{}

	__attribute__((visibility("hidden")))
	void setErrorCode(const char * space, const char * name, int error = 0);

	__attribute__((visibility("hidden")))
	inline void setErrorCode(point_t & point, int error = 0)
	{
		point.error_code = error;
	}

	__attribute__((visibility("hidden")))
	inline void setErrorCode(std::nullptr_t, int = 0)
	{}

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
				if (ptr_ == module_->end) {
					do {
						module_ = module_->next;
					} while ((module_ != nullptr) && (module_->begin == module_->end));

					ptr_ = (module_ != nullptr) ? module_->begin : nullptr;
				}

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
				return (module_ == rhs.module_) && (ptr_ == rhs.ptr_);
			}

			bool operator !=(const iterator& rhs) const
			{
				return (module_ != rhs.module_) || (ptr_ != rhs.ptr_);
			}

		private:
			detail::module_points_t * module_;
			point_t ** ptr_;

			iterator():
				module_(nullptr),
				ptr_(nullptr)
			{}

			iterator(detail::module_points_t * module):
				module_(module)
			{
				while ((module_ != nullptr) && (module_->begin == module_->end)) {
					module_ = module_->next;
				}
				ptr_ = (module_ != nullptr) ? module_->begin : nullptr;
			}

			friend class points_collection;
		};

		iterator begin();
		iterator end();
	};

	extern points_collection points;
}
