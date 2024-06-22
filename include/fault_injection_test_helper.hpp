// -*- compile-command: "cd .. && make test" -*-
#pragma once

#include <optional>

#include <fault_injection.hpp>

namespace avm::fault_injection
{
	class InjectionStateGuard
	{
	public:
		InjectionStateGuard(point_t & point):
			point_{&point},
			reset_state_{!isActive(point)}
		{
			activate(point, getMode(point));
		}

		InjectionStateGuard(point_t & point, mode_t mode):
			point_{&point},
			reset_state_{!isActive(point)},
			old_mode_{getMode(point)}
		{
			activate(point, mode);
		}

		InjectionStateGuard(point_t & point, int error):
			point_{&point},
			reset_state_{!isActive(point)},
			old_error_{getErrorCode(point)}
		{
			setErrorCode(point, error);
			activate(point, getMode(point));
		}

		InjectionStateGuard(point_t & point, mode_t mode, int error):
			point_{&point},
			reset_state_{!isActive(point)},
			old_mode_{getMode(point)},
			old_error_{getErrorCode(point)}
		{
			setErrorCode(point, error);
			activate(point, mode);
		}

		InjectionStateGuard(const char * space, const char * name):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !isActive(*point_);

				activate(*point_, getMode(*point_));
			}
		}

		InjectionStateGuard(const char * space, const char * name, mode_t mode):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !isActive(*point_);
				old_mode_ = getMode(*point_);

				activate(*point_, mode);
			}
		}

		InjectionStateGuard(const char * space, const char * name, int error):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !isActive(*point_);
				old_error_ = getErrorCode(*point_);

				setErrorCode(*point_, error);
				activate(*point_, getMode(*point_));
			}
		}

		InjectionStateGuard(const char * space, const char * name, mode_t mode, int error):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !isActive(*point_);
				old_mode_ = getMode(*point_);
				old_error_ = getErrorCode(*point_);

				setErrorCode(*point_, error);
				activate(*point_, mode);
			}
		}

		InjectionStateGuard(std::nullptr_t):
			point_{nullptr}
		{}

		InjectionStateGuard(InjectionStateGuard &&) = delete;
		InjectionStateGuard(const InjectionStateGuard &) = delete;
		InjectionStateGuard & operator =(InjectionStateGuard &&) = delete;
		InjectionStateGuard & operator =(const InjectionStateGuard &) = delete;

		~InjectionStateGuard()
		{
			if (point_ != nullptr) {
				if (old_error_) {
					setErrorCode(*point_, *old_error_);
				}
				if (old_mode_) {
					setMode(*point_, *old_mode_);
				}
				if (reset_state_) {
					deactivate(*point_);
				}
			}
		}

	private:
		point_t * point_;
		bool reset_state_;
		std::optional<mode_t> old_mode_;
		std::optional<int> old_error_;
	};
}
