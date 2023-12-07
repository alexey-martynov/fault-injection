// -*- compile-command: "cd .. && make test" -*-
#define BOOST_TEST_MODULE fault_injection
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <errno.h>

#include <cstring>
#include <stdexcept>

#include <fault_injection.hpp>
#include <fault_injection_test_helper.hpp>

FAULT_INJECTION_POINT(test, simple, "Simple fault");
FAULT_INJECTION_POINT(test, second, "Second fault");
FAULT_INJECTION_POINT_EX(test2, another, "Another", 0);

static bool isInjected(const std::exception & e)
{
	return std::strcmp(e.what(), "INJECTED") == 0;
}

BOOST_AUTO_TEST_SUITE(iterators)

BOOST_AUTO_TEST_CASE(foreach)
{
	using avm::fault_injection::points;

	BOOST_CHECK_EQUAL(std::distance(points.begin(), points.end()), 3u);

	BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		return (strcmp(point.space, "test") == 0) && (strcmp(point.name, "simple") == 0);
	}) != points.end());
	BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		return (strcmp(point.space, "test") == 0) && (strcmp(point.name, "second") == 0);
	}) != points.end());
	BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		return (strcmp(point.space, "test2") == 0) && (strcmp(point.name, "another") == 0);
	}) != points.end());
}

BOOST_AUTO_TEST_CASE(foreach_const)
{
	const auto points =  avm::fault_injection::points;

	BOOST_CHECK_EQUAL(std::distance(points.begin(), points.end()), 3u);

	BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		return (strcmp(point.space, "test") == 0) && (strcmp(point.name, "simple") == 0);
	}) != points.end());
	BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		return (strcmp(point.space, "test") == 0) && (strcmp(point.name, "second") == 0);
	}) != points.end());
	BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		return (strcmp(point.space, "test2") == 0) && (strcmp(point.name, "another") == 0);
	}) != points.end());
}

BOOST_AUTO_TEST_CASE(const_from_mutable)
{
	using namespace avm::fault_injection;

	points_collection::iterator mutable_iterator = avm::fault_injection::points.begin();
	points_collection::const_iterator const_iterator(mutable_iterator);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(error_code)

BOOST_AUTO_TEST_CASE(no_error)
{
	const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

	BOOST_CHECK_EQUAL(value, 15);
}

BOOST_AUTO_TEST_CASE(error_default)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));

	const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, 0);
}

BOOST_AUTO_TEST_CASE(error_custom)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple), -10);

	const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, -10);
}

BOOST_AUTO_TEST_CASE(error_custom_by_name)
{
	avm::fault_injection::activate("test", "simple");
	avm::fault_injection::setErrorCode("test", "simple", -10);

	const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

	avm::fault_injection::setErrorCode("test", "simple");
	avm::fault_injection::deactivate("test", "simple");

	BOOST_CHECK_EQUAL(value, -10);
}

BOOST_AUTO_TEST_CASE(error_multiple)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::multiple);

	const int value1 = FAULT_INJECT_ERROR_CODE(test, simple, 15);
	const int value2 = FAULT_INJECT_ERROR_CODE(test, simple, 16);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value1, 0);
	BOOST_CHECK_EQUAL(value2, 0);
}

BOOST_AUTO_TEST_CASE(error_oneshot)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::oneshot);

	const int value1 = FAULT_INJECT_ERROR_CODE(test, simple, 15);
	const int value2 = FAULT_INJECT_ERROR_CODE(test, simple, 16);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value1, 0);
	BOOST_CHECK_EQUAL(value2, 16);
}

BOOST_AUTO_TEST_CASE(error_condition_false)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));
	bool enabled = false;

	const int value1 = FAULT_INJECT_ERROR_CODE_IF(test, simple, enabled, 15);

	BOOST_CHECK_EQUAL(value1, 15);
}

BOOST_AUTO_TEST_CASE(error_condition_true)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));
	bool enabled = true;

	const int value1 = FAULT_INJECT_ERROR_CODE_IF(test, simple, enabled, 15);

	BOOST_CHECK_EQUAL(value1, 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(guard)

BOOST_AUTO_TEST_CASE(error_default)
{
	{
		avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));

		const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

		BOOST_CHECK_EQUAL(value, 0);
	}

	BOOST_CHECK(!avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(test, simple)));
}

BOOST_AUTO_TEST_CASE(error_custom)
{
	{
		avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple), -10);

		const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

		BOOST_CHECK_EQUAL(value, -10);
	}

	BOOST_CHECK(!avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(test, simple)));
	BOOST_CHECK_EQUAL(avm::fault_injection::getErrorCode(FAULT_INJECTION_POINT_REF(test, simple)), 0);
}

BOOST_AUTO_TEST_CASE(error_custom_by_name)
{
	{
		avm::fault_injection::InjectionStateGuard guard("test", "simple", -10);

		const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

		BOOST_CHECK_EQUAL(value, -10);
	}

	BOOST_CHECK(!avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(test, simple)));
	BOOST_CHECK_EQUAL(avm::fault_injection::getErrorCode(FAULT_INJECTION_POINT_REF(test, simple)), 0);
}

BOOST_AUTO_TEST_CASE(error_oneshot)
{
	{
		avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::oneshot);


		const int value1 = FAULT_INJECT_ERROR_CODE(test, simple, 15);
		const int value2 = FAULT_INJECT_ERROR_CODE(test, simple, 16);

		BOOST_CHECK_EQUAL(value1, 0);
		BOOST_CHECK_EQUAL(value2, 16);
	}

	BOOST_CHECK(!avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(test, simple)));
}

BOOST_AUTO_TEST_CASE(error_oneshot_custom_error)
{
	{
		avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::oneshot, -10);


		const int value1 = FAULT_INJECT_ERROR_CODE(test, simple, 15);
		const int value2 = FAULT_INJECT_ERROR_CODE(test, simple, 16);

		BOOST_CHECK_EQUAL(value1, -10);
		BOOST_CHECK_EQUAL(value2, 16);
	}

	BOOST_CHECK(!avm::fault_injection::isActive(FAULT_INJECTION_POINT_REF(test, simple)));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(errno_code)

BOOST_AUTO_TEST_CASE(no_error)
{
	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO(test, simple, action());

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_default)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO(test, simple, action());

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, -1);
	BOOST_CHECK_EQUAL(errno, 0);
	BOOST_CHECK(!called);
}

BOOST_AUTO_TEST_CASE(error_custom)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple), EAGAIN);

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO(test, simple, action());

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, -1);
	BOOST_CHECK_EQUAL(errno, EAGAIN);
	BOOST_CHECK(!called);
}

BOOST_AUTO_TEST_CASE(error_default_ex)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO_EX(test, simple, action(), -10);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, -10);
	BOOST_CHECK_EQUAL(errno, 0);
	BOOST_CHECK(!called);
}

BOOST_AUTO_TEST_CASE(error_custom_ex)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple), EAGAIN);

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO_EX(test, simple, action(), -10);

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, -10);
	BOOST_CHECK_EQUAL(errno, EAGAIN);
	BOOST_CHECK(!called);
}

BOOST_AUTO_TEST_CASE(error_multiple)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::multiple);

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value1 = FAULT_INJECT_ERRNO(test, simple, action());
	const int value2 = FAULT_INJECT_ERRNO(test, simple, action());

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value1, -1);
	BOOST_CHECK_EQUAL(value2, -1);
	BOOST_CHECK(!called);
}

BOOST_AUTO_TEST_CASE(error_oneshot)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::oneshot);

	int called = 0;
	auto action = [&called] {
		++called;
		return 0;
	};
	const int value1 = FAULT_INJECT_ERRNO(test, simple, action());
	const int value2 = FAULT_INJECT_ERRNO(test, simple, action());

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value1, -1);
	BOOST_CHECK_EQUAL(value2, 0);
	BOOST_CHECK_EQUAL(called, 1);
}

BOOST_AUTO_TEST_CASE(error_default_condition_false)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	bool enabled = false;
	const int value = FAULT_INJECT_ERRNO_IF(test, simple, enabled, action());

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_default_condition_true)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	bool enabled = true;
	const int value = FAULT_INJECT_ERRNO_IF(test, simple, enabled, action());

	BOOST_CHECK_EQUAL(value, -1);
	BOOST_CHECK_EQUAL(errno, 0);
	BOOST_CHECK(!called);
}

BOOST_AUTO_TEST_CASE(error_default_ex_condition_false)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	bool enabled = false;
	const int value = FAULT_INJECT_ERRNO_IF_EX(test, simple, enabled, action(), -10);

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_default_ex_condition_true)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	bool enabled = true;
	const int value = FAULT_INJECT_ERRNO_IF_EX(test, simple, enabled, action(), -10);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EQUAL(value, -10);
	BOOST_CHECK_EQUAL(errno, 0);
	BOOST_CHECK(!called);
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(exception)

BOOST_AUTO_TEST_CASE(no_error)
{
	BOOST_CHECK_NO_THROW(FAULT_INJECT_EXCEPTION(test, simple, std::runtime_error("INJECTED")));
}

BOOST_AUTO_TEST_CASE(error)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));

	BOOST_CHECK_EXCEPTION(FAULT_INJECT_EXCEPTION(test, simple, std::runtime_error("INJECTED")), std::runtime_error, isInjected);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));
}

BOOST_AUTO_TEST_CASE(error_multiple)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::multiple);

	BOOST_CHECK_EXCEPTION(FAULT_INJECT_EXCEPTION(test, simple, std::runtime_error("INJECTED")), std::runtime_error, isInjected);
	BOOST_CHECK_EXCEPTION(FAULT_INJECT_EXCEPTION(test, simple, std::runtime_error("INJECTED")), std::runtime_error, isInjected);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));
}

BOOST_AUTO_TEST_CASE(error_oneshot)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), avm::fault_injection::mode_t::oneshot);

	BOOST_CHECK_EXCEPTION(FAULT_INJECT_EXCEPTION(test, simple, std::runtime_error("INJECTED")), std::runtime_error, isInjected);
	BOOST_CHECK_NO_THROW(FAULT_INJECT_EXCEPTION(test, simple, std::runtime_error("INJECTED")));

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, simple));
}

BOOST_AUTO_TEST_CASE(error_condition_false)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));
	bool enabled = false;

	BOOST_CHECK_NO_THROW(FAULT_INJECT_EXCEPTION_IF(test, simple, enabled, std::runtime_error("INJECTED")));
}

BOOST_AUTO_TEST_CASE(error_condition_true)
{
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));
	bool enabled = true;

	BOOST_CHECK_EXCEPTION(FAULT_INJECT_EXCEPTION_IF(test, simple, enabled, std::runtime_error("INJECTED")), std::runtime_error, isInjected);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(action)

BOOST_AUTO_TEST_CASE(no_action)
{
	[[maybe_unused]] bool invoked = false;

	FAULT_INJECT_ACTION(test, simple, do { invoked = true; } while (false));

	BOOST_CHECK(!invoked);
}

BOOST_AUTO_TEST_CASE(with_action)
{
	[[maybe_unused]] bool invoked = false;
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, simple));

	FAULT_INJECT_ACTION(test, simple, do { invoked = true; } while (false));

	BOOST_CHECK(invoked);
}

BOOST_AUTO_TEST_SUITE_END()
