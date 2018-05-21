// example_tests.cpp
#include "catch.hpp"

struct SomethingWeWantToTest {
	SomethingWeWantToTest() : m_value(1) {}
	int m_value;
};

TEST_CASE("Simple example") {
	SomethingWeWantToTest testObject;

	SECTION("First section, fails") {
		REQUIRE(testObject.m_value == 0);
	}
	SECTION("Second section, works") {
		REQUIRE(testObject.m_value == 1);
	}
}