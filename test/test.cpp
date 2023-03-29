// -*- compile-command: "cd .. && make test" -*-
#define BOOST_TEST_MODULE fault_injection
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <errno.h>

#include <cstring>
#include <stdexcept>

#include <fault_injection.hpp>

FAULT_INJECTION_POINT(test, simple);
FAULT_INJECTION_POINT(test, second);
FAULT_INJECTION_POINT(test2, another);

static bool isInjected(const std::exception & e)
{
	return std::strcmp(e.what(), "INJECTED") == 0;
}

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

	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);

	BOOST_CHECK_EQUAL(value, 0);
}

BOOST_AUTO_TEST_CASE(error_custom)
{
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple), -10);

	const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

	avm::fault_injection::setErrorCode(FAULT_INJECTION_POINT_REF(test, simple));
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);

	BOOST_CHECK_EQUAL(value, -10);
}

BOOST_AUTO_TEST_CASE(error_custom_by_name)
{
	avm::fault_injection::activate("test", "simple");
	avm::fault_injection::setErrorCode("test", "simple", -10);

	const int value = FAULT_INJECT_ERROR_CODE(test, simple, 15);

	avm::fault_injection::setErrorCode("test", "simple");
	avm::fault_injection::activate("test", "simple", false);

	BOOST_CHECK_EQUAL(value, -10);
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

	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);

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
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);

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

	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);

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
	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);

	BOOST_CHECK_EQUAL(value, -10);
	BOOST_CHECK_EQUAL(errno, EAGAIN);
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

	avm::fault_injection::activate(FAULT_INJECTION_POINT_REF(test, simple), false);
}

BOOST_AUTO_TEST_SUITE_END()

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
BOOST_AUTO_TEST_SUITE_END()
