// -*- compile-command: "cd .. && make test" -*-
#pragma once

#if !defined(FAULT_INJECTION_HAS_THREADS)
#define FAULT_INJECTION_HAS_THREADS 1
#define FAULT_INJECTION_READ(var) ((var).load(std::memory_order_acquire))
#define FAULT_INJECTION_WRITE(var, value) ((var).store(value, std::memory_order_release), value)
#else
#define FAULT_INJECTION_READ(var) (var)
#define FAULT_INJECTION_WRITE(var, value) (var) = (value)
#endif

#if FAULT_INJECTION_HAS_THREADS > 0
#include <atomic>
#endif
#include <cstdint>
#include <cassert>
#include <iterator>
#include <utility>

namespace avm::fault_injection
{
	enum class mode_t: std::uint8_t {
		multiple,
		oneshot
	};

	struct point_t
	{
		const char * const space;
		const char * const name;
#if FAULT_INJECTION_HAS_THREADS > 0
		std::atomic<int> error_code;
		std::atomic<bool> active;
		std::atomic<mode_t> mode;
#else
		int error_code;
		bool active;
		mode_t mode;
#endif
	};

	namespace detail
	{
		struct module_points_t
		{
			module_points_t * next;
			avm::fault_injection::point_t ** const begin;
			avm::fault_injection::point_t ** const end;
			bool registered;
		};
	}


#if (FAULT_INJECTIONS_ENABLED > 0) || (FAULT_INJECTIONS_DEFINITIONS > 0)

#define FAULT_INJECTION_POINT_REF(space, name) ::space::fault_injection_point_##name

#if defined(__APPLE__)
#define FAULT_INJECTION_POINT_EX(space, name, error_code)	  \
	namespace space { \
		::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used)) = { #space, #name, error_code, false, ::avm::fault_injection::mode_t::multiple }; \
		static ::avm::fault_injection::point_t * fault_injection_point_##name##_ptr __attribute__((used,section("__DATA,__faults"))) = &FAULT_INJECTION_POINT_REF(space, name); \
	}
#elif defined(__linux__)
#define FAULT_INJECTION_POINT_EX(space, name, error_code)	  \
	namespace space { \
		::avm::fault_injection::point_t fault_injection_point_##name __attribute__((used)) = { #space, #name, error_code, false, ::avm::fault_injection::mode_t::multiple }; \
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

#define FAULT_INJECTION_ONESHOT(space, name) ((FAULT_INJECTION_READ(FAULT_INJECTION_POINT_REF(space, name).mode) == ::avm::fault_injection::mode_t::multiple) \
			? true \
			: FAULT_INJECTION_WRITE(FAULT_INJECTION_POINT_REF(space, name).active, false)) \

#define FAULT_INJECT_ERROR_CODE_IF(space, name, condition, action) ((::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name)) && (condition)) \
			? (FAULT_INJECTION_ONESHOT(space, name), FAULT_INJECTION_READ(FAULT_INJECTION_POINT_REF(space, name).error_code)) \
			: (action))
#define FAULT_INJECT_ERRNO_IF_EX(space, name, condition, action, result) ((::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name)) && (condition)) \
			? (FAULT_INJECTION_ONESHOT(space, name), (errno = FAULT_INJECTION_READ(FAULT_INJECTION_POINT_REF(space, name).error_code)), (result)) \
			: (action))
#define FAULT_INJECT_EXCEPTION_IF(space, name, condition, exception) do { \
		if (::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name)) && (condition)) { \
			static_cast<void>(FAULT_INJECTION_ONESHOT(space, name)); \
			throw (exception); \
		} \
	} while (false)

#define FAULT_INJECT_ACTION(space, name, action) do {	  \
	if (::avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(space, name))) { \
		static_cast<void>(FAULT_INJECTION_ONESHOT(space, name)); \
		action; \
	} \
} while (false)

#else

#define FAULT_INJECT_ERROR_CODE_IF(space, name, condition, action) (action)
#define FAULT_INJECT_ERRNO_IF_EX(space, name, condition, action, result) (action)
#define FAULT_INJECT_EXCEPTION_IF(space, name, condition, exception)
#define FAULT_INJECT_ACTION(space, name, action)

#endif

#define FAULT_INJECT_ERROR_CODE(space, name, action) FAULT_INJECT_ERROR_CODE_IF(space, name, true, action)
#define FAULT_INJECT_ERRNO(space, name, action) FAULT_INJECT_ERRNO_IF_EX(space, name, true, action, -1)
#define FAULT_INJECT_ERRNO_EX(space, name, action, result) FAULT_INJECT_ERRNO_IF_EX(space, name, true, action, result)
#define FAULT_INJECT_ERRNO_IF(space, name, condition, action) FAULT_INJECT_ERRNO_IF_EX(space, name, condition, action, -1)
#define FAULT_INJECT_EXCEPTION(space, name, exception) FAULT_INJECT_EXCEPTION_IF(space, name, true, exception)

	__attribute__((visibility("hidden")))
	void registerModule();

	__attribute__((visibility("hidden")))
	point_t * find(const char * space, const char * name);

	__attribute__((visibility("hidden")))
	inline bool isActive(const char * space, const char * name)
	{
		point_t * point = find(space, name);

		return (point != nullptr) ? FAULT_INJECTION_READ(point->active) : false;
	}

	__attribute__((visibility("hidden")))
	inline bool isActive(const point_t & point)
	{
		if (FAULT_INJECTION_READ(point.active)) {
			registerModule();
			return true;
		}

		return false;
	}

	__attribute__((visibility("hidden")))
	inline bool isActive(std::nullptr_t)
	{
		return false;
	}

	__attribute__((visibility("hidden")))
	inline void activate(const char * space, const char * name, mode_t mode = mode_t::multiple)
	{
		point_t * point = find(space, name);

		if (point != nullptr) {
			FAULT_INJECTION_WRITE(point->mode, mode);
			FAULT_INJECTION_WRITE(point->active, true);
		}
	}

	__attribute__((visibility("hidden")))
	inline void activate(point_t & point, mode_t mode = mode_t::multiple)
	{
		FAULT_INJECTION_WRITE(point.mode, mode);
		FAULT_INJECTION_WRITE(point.active, true);
	}

	__attribute__((visibility("hidden")))
	inline void activate(std::nullptr_t, mode_t = mode_t::multiple)
	{}

	__attribute__((visibility("hidden")))
	inline void deactivate(const char * space, const char * name)
	{
		point_t * point = find(space, name);

		if (point != nullptr) {
			point->active = false;
		}
	}

	__attribute__((visibility("hidden")))
	inline void deactivate(point_t & point)
	{
		FAULT_INJECTION_WRITE(point.active, false);
	}

	__attribute__((visibility("hidden")))
	inline void deactivate(std::nullptr_t)
	{}

	__attribute__((visibility("hidden")))
	inline void setErrorCode(const char * space, const char * name, int error = 0)
	{
		point_t * point = find(space, name);

		if (point != nullptr) {
			FAULT_INJECTION_WRITE(point->error_code, error);
		}
	}

	__attribute__((visibility("hidden")))
	inline void setErrorCode(point_t & point, int error = 0)
	{
		FAULT_INJECTION_WRITE(point.error_code, error);
	}

	__attribute__((visibility("hidden")))
	inline void setErrorCode(std::nullptr_t, int = 0)
	{}

	__attribute__((visibility("hidden")))
	inline int getErrorCode(const char * space, const char * name)
	{
		point_t * point = find(space, name);

		return (point != nullptr) ? FAULT_INJECTION_READ(point->error_code) : 0;
	}

	__attribute__((visibility("hidden")))
	inline int getErrorCode(point_t & point)
	{
		return FAULT_INJECTION_READ(point.error_code);
	}

	__attribute__((visibility("hidden")))
	inline int getErrorCode(std::nullptr_t)
	{
		return 0;
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
				do {
					++ptr_;
					if (ptr_ == module_->end) {
						do {
							module_ = module_->next;
						} while ((module_ != nullptr) && (module_->begin == module_->end));

						ptr_ = (module_ != nullptr) ? module_->begin : nullptr;
					}
				} while ((ptr_ != nullptr) && (*ptr_ == nullptr));

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

				if ((ptr_ != nullptr) && (*ptr_ == nullptr)) {
					// Skip fake instance
					++*this;
				}
			}

			friend class points_collection;
		};

		class const_iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = point_t;
			using pointer           = const point_t *;
			using reference         = const point_t &;

			const_iterator(const iterator& rhs):
				module_(rhs.module_),
				ptr_(rhs.ptr_)
			{}

			const_iterator(iterator&& rhs):
				module_(std::exchange(rhs.module_, nullptr)),
				ptr_(std::exchange(rhs.ptr_, nullptr))
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

			const_iterator& operator ++()
			{
				do {
					++ptr_;
					if (ptr_ == module_->end) {
						do {
							module_ = module_->next;
						} while ((module_ != nullptr) && (module_->begin == module_->end));

						ptr_ = (module_ != nullptr) ? module_->begin : nullptr;
					}
				} while ((ptr_ != nullptr) && (*ptr_ == nullptr));

				return *this;
			}

			const_iterator operator ++(int)
			{
				const_iterator result{*this};

				++ptr_;

				return result;
			}

			bool operator ==(const const_iterator& rhs) const
			{
				return (module_ == rhs.module_) && (ptr_ == rhs.ptr_);
			}

			bool operator !=(const const_iterator& rhs) const
			{
				return (module_ != rhs.module_) || (ptr_ != rhs.ptr_);
			}

		private:
			detail::module_points_t * module_;
			point_t ** ptr_;

			const_iterator():
				module_(nullptr),
				ptr_(nullptr)
			{}

			const_iterator(detail::module_points_t * module):
				module_(module)
			{
				while ((module_ != nullptr) && (module_->begin == module_->end)) {
					module_ = module_->next;
				}
				ptr_ = (module_ != nullptr) ? module_->begin : nullptr;

				if ((ptr_ != nullptr) && (*ptr_ == nullptr)) {
					// Skip fake instance
					++*this;
				}
			}

			friend class points_collection;
		};

		const_iterator begin() const;
		iterator begin();
		const_iterator end() const;
		iterator end();
	};

	extern points_collection points;
}
