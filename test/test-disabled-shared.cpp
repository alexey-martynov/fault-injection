// -*- compile-command: "cd .. && make test" -*-
#define BOOST_TEST_MODULE fault_injection_shared
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <errno.h>

#include <cstring>
#include <stdexcept>

#include <fault_injection.hpp>
#include <fault_injection_test_helper.hpp>

FAULT_INJECTION_POINT(test, point1, "Point 1");
FAULT_INJECTION_POINT(test, point2, "Point 2");

void executeWithInjection();
void executeV0WithInjection();

static bool isInjected(const std::exception & e)
{
	return std::strcmp(e.what(), "INJECTED") == 0;
}

BOOST_AUTO_TEST_SUITE(dynamiclib)

BOOST_AUTO_TEST_CASE(foreach)
{
	  using avm::fault_injection::points;

	  BOOST_CHECK_EQUAL(std::distance(points.begin(), points.end()), 2u);

	  BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		  return (strcmp(getSpace(point), "test") == 0) && (strcmp(getName(point), "point1") == 0);
	  }) == points.end());
	  BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		  return (strcmp(getSpace(point), "test") == 0) && (strcmp(getName(point), "point2") == 0);
	  }) == points.end());
	  BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		  return (strcmp(getSpace(point), "lib") == 0) && (strcmp(getName(point), "point1") == 0);
	  }) != points.end());
	  BOOST_CHECK(std::find_if(points.begin(), points.end(), [](const auto & point) {
		  return (strcmp(getSpace(point), "lib") == 0) && (strcmp(getName(point), "point_v0") == 0);
	  }) != points.end());
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(disabled_error_code)

BOOST_AUTO_TEST_CASE(no_error)
{
	const int value = FAULT_INJECT_ERROR_CODE(test, point1, 15);

	BOOST_CHECK_EQUAL(value, 15);
}

BOOST_AUTO_TEST_CASE(error_default)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));

	const int value = FAULT_INJECT_ERROR_CODE(test, point1, 15);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_EQUAL(value, 15);
}

BOOST_AUTO_TEST_CASE(error_custom)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, point1), -10);

	const int value = FAULT_INJECT_ERROR_CODE(test, point1, 15);

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, point1));
	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_EQUAL(value, 15);
}

BOOST_AUTO_TEST_CASE(error_custom_by_name)
{
	avm::fault_injection::activate("test", "point1");
	avm::fault_injection::setErrorCode("test", "point1", -10);

	const int value = FAULT_INJECT_ERROR_CODE(test, point1, 15);

	avm::fault_injection::setErrorCode("test", "point1");
	avm::fault_injection::deactivate("test", "point1");

	BOOST_CHECK_EQUAL(value, 15);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(disabled_errno_code)

BOOST_AUTO_TEST_CASE(no_error)
{
	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO(test, point1, action());

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_default)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO(test, point1, action());

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_custom)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, point1), EAGAIN);

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO(test, point1, action());

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, point1));
	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_default_ex)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO_EX(test, point1, action(), -10);

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(error_custom_ex)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, point1), EAGAIN);

	bool called = false;
	auto action = [&called] {
		called = true;
		return 0;
	};
	const int value = FAULT_INJECT_ERRNO_EX(test, point1, action(), -10);

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, point1));
	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_EQUAL(value, 0);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(disabled_exception)

BOOST_AUTO_TEST_CASE(no_error)
{
	BOOST_CHECK_NO_THROW(FAULT_INJECT_EXCEPTION(test, point1, std::runtime_error("INJECTED")));
}

BOOST_AUTO_TEST_CASE(error)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, point1));

	BOOST_CHECK_NO_THROW(FAULT_INJECT_EXCEPTION(test, point1, std::runtime_error("INJECTED")));

	avm::fault_injection::deactivate(FAULT_INJECTION_POINT_REF(test, point1));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(disabled_action)

BOOST_AUTO_TEST_CASE(no_action)
{
	bool invoked = false;

	FAULT_INJECT_ACTION(test, point1, do { invoked = true } while (false));

	BOOST_CHECK(!invoked);
}

BOOST_AUTO_TEST_CASE(with_action)
{
	bool invoked = false;
	avm::fault_injection::InjectionStateGuard guard(FAULT_INJECTION_POINT_REF(test, point1));

	FAULT_INJECT_ACTION(test, point1, do { invoked = true } while (false));

	BOOST_CHECK(!invoked);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(shared_lib)

BOOST_AUTO_TEST_CASE(no_error)
{
	BOOST_CHECK_NO_THROW(executeWithInjection());
}

BOOST_AUTO_TEST_CASE(no_error_v0)
{
	BOOST_CHECK_NO_THROW(executeV0WithInjection());
}

BOOST_AUTO_TEST_CASE(error)
{
	avm::fault_injection::activate("lib", "point1");

	BOOST_CHECK_EXCEPTION(executeWithInjection(), std::runtime_error, isInjected);

	avm::fault_injection::deactivate("lib", "point1");
}

BOOST_AUTO_TEST_CASE(error_v0)
{
	avm::fault_injection::activate("lib", "point_v0");

	BOOST_CHECK_EXCEPTION(executeV0WithInjection(), std::runtime_error, isInjected);

	avm::fault_injection::deactivate("lib", "point_v0");
}

BOOST_AUTO_TEST_SUITE_END()
