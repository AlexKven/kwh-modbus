#include "pch.h"
#include "fakeit.hpp"
#include "../kwh-modbus/libraries/bitFunctions/BitFunctions.hpp"
#include "test_helpers.h"
#include "../kwh-modbus/noArduino/ArduinoMacros.h"
#include "../kwh-modbus/libraries/debugMacros/DebugMacros.h"

DEBUG_CATEGORY(CategoryOne|CategoryTwo|CategoryThree)

TEST_TRAITS(DebugMacroTests, CategoryEnabled_One,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("CategoryOne");
	ASSERT_TRUE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_Two,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("CategoryTwo");
	ASSERT_TRUE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_Three,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("CategoryThree");
	ASSERT_TRUE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_Blank,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("");
	ASSERT_FALSE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_Pipe,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("|");
	ASSERT_TRUE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_UnrelatedCategoryName,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("FooBar");
	ASSERT_FALSE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_SimilarCategoryName,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("CategoryFour");
	ASSERT_FALSE(result);
}

TEST_TRAITS(DebugMacroTests, CategoryEnabled_SubsetCategoryName,
	Type, Unit, Threading, Single, Determinism, Static, Case, Typical)
{
	auto result = isDebugCategoryEnabled("Category");
	ASSERT_FALSE(result);
}