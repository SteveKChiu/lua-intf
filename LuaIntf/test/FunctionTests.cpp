#include "LuaIntf/test/Common.h"

void PointerTest(TestClass* test)
{
	test->number = 42;
}

void ReferenceTest(TestClass& test)
{
	test.number = 24;
}

int number = 0;

void SetNum(int value)
{
	number = value;
}

SCENARIO("Global functions/variables are bound to a Lua state.", "[binding]")
{
	LuaContext state;

	LuaBinding(state).beginClass<TestClass>("TestClass")
		.addConstructor(LUA_ARGS())
		.addVariable("number", &TestClass::number)
	.endClass()
	.beginModule("Module")
		.addFunction("PointerTest", &PointerTest)
		.addFunction("ReferenceTest", &ReferenceTest)
		.addProperty("number", [](){ return number; }, &SetNum)
	.endModule();

	WHEN("A C++ object is used as an argument to a Lua function")
	{
		TestClass test;

		WHEN("the function expects a reference")
		{
			state.getGlobal("Module.ReferenceTest")(test);
			REQUIRE(test.number == 24);
		}

		WHEN("the function expects a pointer")
		{
			state.getGlobal("Module.PointerTest")(&test);
			REQUIRE(test.number == 42);
		}
	}

	WHEN("A Lua object is used as an argument to a C++ function")
	{
		state.doString("test = TestClass()");

		WHEN("the function expects a reference")
		{
			ReferenceTest(state.getGlobal<TestClass&>("test"));
			REQUIRE(state.getGlobal<int>("test.number") == 24);
		}

		WHEN("the function expects a pointer")
		{
			PointerTest(state.getGlobal<TestClass*>("test"));
			REQUIRE(state.getGlobal<int>("test.number") == 42);
		}
	}

	WHEN("A Lua object is used as an argument to a Lua function")
	{
		state.doString("test = TestClass()");

		WHEN("the function expects a reference")
		{
			state.doString("Module.ReferenceTest(test)");
			REQUIRE(state.getGlobal<int>("test.number") == 24);
		}

		WHEN("the function expects a pointer")
		{
			state.doString("Module.PointerTest(test)");
			REQUIRE(state.getGlobal<int>("test.number") == 42);
		}
	}

	WHEN("A global variable is accessed")
	{
		THEN("Lua and C++ are kept in sync")
		{
			state.doString("Module.number = 90");
			REQUIRE(number == 90);

			number = 45;
			REQUIRE(state.getGlobal<int>("Module.number") == 45);
		}
	}
}
