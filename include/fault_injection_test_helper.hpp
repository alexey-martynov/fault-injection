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
			reset_state_{!FAULT_INJECTION_READ(point.active)}
		{
			FAULT_INJECTION_WRITE(point.active, true);
		}

		InjectionStateGuard(point_t & point, mode_t mode):
			point_{&point},
			reset_state_{!FAULT_INJECTION_READ(point.active)},
			old_mode_{FAULT_INJECTION_READ(point.mode)}
		{
			FAULT_INJECTION_WRITE(point.mode, mode);
			FAULT_INJECTION_WRITE(point.active, true);
		}

		InjectionStateGuard(point_t & point, int error):
			point_{&point},
			reset_state_{!FAULT_INJECTION_READ(point.active)},
			old_error_{FAULT_INJECTION_READ(point.error_code)}
		{
			FAULT_INJECTION_WRITE(point.error_code, error);
			FAULT_INJECTION_WRITE(point.active, true);
		}

		InjectionStateGuard(point_t & point, mode_t mode, int error):
			point_{&point},
			reset_state_{!FAULT_INJECTION_READ(point.active)},
			old_mode_{FAULT_INJECTION_READ(point.mode)},
			old_error_{FAULT_INJECTION_READ(point.error_code)}
		{
			FAULT_INJECTION_WRITE(point.mode, mode);
			FAULT_INJECTION_WRITE(point.error_code, error);
			FAULT_INJECTION_WRITE(point.active, true);
		}

		InjectionStateGuard(const char * space, const char * name):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !FAULT_INJECTION_READ(point_->active);

				FAULT_INJECTION_WRITE(point_->active, true);
			}
		}

		InjectionStateGuard(const char * space, const char * name, mode_t mode):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !FAULT_INJECTION_READ(point_->active);
				old_mode_ = FAULT_INJECTION_READ(point_->mode);

				FAULT_INJECTION_WRITE(point_->mode, mode);
				FAULT_INJECTION_WRITE(point_->active, true);
			}
		}

		InjectionStateGuard(const char * space, const char * name, int error):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !FAULT_INJECTION_READ(point_->active);
				old_error_ = FAULT_INJECTION_READ(point_->error_code);

				FAULT_INJECTION_WRITE(point_->error_code, error);
				FAULT_INJECTION_WRITE(point_->active, true);
			}
		}

		InjectionStateGuard(const char * space, const char * name, mode_t mode, int error):
			point_{find(space, name)},
			reset_state_{false}
		{
			if (point_ != nullptr) {
				reset_state_ = !FAULT_INJECTION_READ(point_->active);
				old_mode_ = FAULT_INJECTION_READ(point_->mode);
				old_error_ = FAULT_INJECTION_READ(point_->error_code);

				FAULT_INJECTION_WRITE(point_->mode, mode);
				FAULT_INJECTION_WRITE(point_->error_code, error);
				FAULT_INJECTION_WRITE(point_->active, true);
			}
		}

		InjectionStateGuard(InjectionStateGuard &&) = delete;
		InjectionStateGuard(const InjectionStateGuard &) = delete;
		InjectionStateGuard & operator =(InjectionStateGuard &&) = delete;
		InjectionStateGuard & operator =(const InjectionStateGuard &) = delete;

		~InjectionStateGuard()
		{
			if (point_ != nullptr) {
				if (old_error_) {
					FAULT_INJECTION_WRITE(point_->error_code, *old_error_);
				}
				if (old_mode_) {
					FAULT_INJECTION_WRITE(point_->mode, *old_mode_);
				}
				if (reset_state_) {
					FAULT_INJECTION_WRITE(point_->active, false);
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
