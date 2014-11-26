#include "LuaIntf/test/Common.h"

int TestClass::static_number = 3;
std::string TestClass::static_string = "blah blah";
float TestClass::static_other = 3.14f;
const int TestClass::static_const = 1;

int TestClass::instance_counter = 0;

SCENARIO("Static functions of classes can be bound", "[binding][class]")
{
	LuaContext state;
    TestClass::Register(state);

	WHEN("static variables are modified in C++")
	{
		TestClass::static_number = 5;
		THEN("the Lua class is affected")
		{
			REQUIRE(state.getGlobal<int>("TestClass.number"));
		}
	}
	WHEN("static variables are modified in Lua")
	{
		state.doString("TestClass.number = 3");
		THEN("the C++ class is affected")
		{
			REQUIRE(TestClass::static_number == 3);
		}
	}
}

SCENARIO("An object is created in Lua", "[binding][class]")
{
	LuaContext state;
    TestClass::Register(state);

	state.doString("test = TestClass(5)");

	WHEN("the object is accessed in C++")
	{
		THEN("the variables match")
		{
			REQUIRE(state.getGlobal<TestClass>("test").number == 5);
		}
		THEN("variables changed in C++ change affect the Lua object")
		{
			TestClass& test = state.getGlobal<TestClass&>("test");
			test.number = 8;
			REQUIRE(state.getGlobal<int>("test.number") == 8);
		}
	}

// Lua throws C exceptions which cannot be caught on the C++ side when not compiled as C++
#if LUAINTF_LINK_LUA_COMPILED_IN_CXX
	WHEN("constant variables are used")
	{
		THEN("values are read only")
		{
			REQUIRE_THROWS(state.doString("test.const = 5"));
			REQUIRE(state.getGlobal<int>("test.const") == 8);
		}
	}
#endif

	WHEN("the object is destroyed")
	{
		state.doString("test = nil");
		state.gc();
		THEN("the C++ construct is also destroyed")
		{
			REQUIRE(TestClass::GetCount() == 0);
		}
	}
}

SCENARIO("An object is created in C++", "[binding][class]")
{
	LuaContext state;
    TestClass::Register(state);

	GIVEN("the object is passed by reference")
	{
		TestClass test;
		state.setGlobal<TestClass&>("test", test);

		THEN("the object count increases")
		{
			REQUIRE(TestClass::GetCount() == 1);
		}

		THEN("Lua interaction modifies the C++ object")
		{
			state.doString( "test.number = 5\n"
							"test.string = \"hello\"\n"
							"test.float = 0.75");

			REQUIRE(test.number == 5);
			REQUIRE(test.string == "hello");
			REQUIRE(test.other == 0.75f);
		}
	}

	GIVEN("the object is passed by pointer")
	{
		TestClass* test = new TestClass();
		state.setGlobal("test", test);

		THEN("the object count does not increase")
		{
			REQUIRE(TestClass::GetCount() == 1);
		}

		THEN("Lua interaction modifies the C++ object")
		{
			state.doString( "test.number = 5\n"
							"test.string = \"hello\"\n"
							"test.float = 0.75");

			REQUIRE(test->number == 5);
			REQUIRE(test->string == "hello");
			REQUIRE(test->other == 0.75f);
		}

		THEN("deletion of Lua value does not destroy the object")
		{
			state.doString("test = nil");
			state.gc();

			// Lazy way to check that the object exists.
			REQUIRE(test->string == "default");
		}

		delete test;

		THEN("deletion of the pointer does not nil the Lua variable")
		{
			REQUIRE(state.getGlobal("test"));
		}
	}

	GIVEN("the object is passed by shared pointer")
	{
		{
			std::shared_ptr<TestClass> test (new TestClass);
			state.setGlobal("test", test);

			THEN("the object count does not change")
			{
				REQUIRE(TestClass::GetCount() == 1);
			}

			THEN("Lua interaction modifies the C++ object")
			{
				state.doString( "test.number = 5\n"
								"test.string = \"hello\"\n"
								"test.float = 0.75");

				REQUIRE(test->number == 5);
				REQUIRE(test->string == "hello");
				REQUIRE(test->other == 0.75f);
			}

			THEN("deletion of Lua value does not destroy the object")
			{
				state.doString("test = nil");
				state.gc();

				REQUIRE(test.unique());
			}
		}

		THEN("deletion of the shared pointer does not destroy the Lua variable")
		{
			REQUIRE(state.getGlobal("test"));
		}
	}
}

struct OtherClass
{
	TestClass* mine;

	OtherClass(TestClass* thing): mine(thing) {}
	OtherClass(const OtherClass& that): mine(that.mine) {}
	OtherClass(OtherClass&& that): mine(std::move(that.mine)) {}
	OtherClass& operator=(const OtherClass&) = delete;
	OtherClass& operator=(OtherClass&&) = delete;

	std::string Print() const
	{
		return mine->Print();
	}
};

SCENARIO("Classes constructed with other classes are bound.", "[binding][class]")
{
	LuaContext state;
    TestClass::Register(state);
	LuaBinding(state).beginClass<OtherClass>("OtherClass")
		.addConstructor(LUA_ARGS(TestClass*))
		.addFunction("Print", &OtherClass::Print)
	.endClass();

	WHEN("A Lua object is created with a C++ object")
	{
		TestClass test;
		state.setGlobal("contained", test);
		state.doString("test = OtherClass(contained):Print()");

		REQUIRE(state.getGlobal<std::string>("test") == test.Print());
	}

	WHEN("A C++ object is created with a Lua object")
	{
		state.doString("test = TestClass()");
		OtherClass test(state.getGlobal<TestClass*>("test"));
		state.doString("other = test:Print()");

		REQUIRE(state.getGlobal<std::string>("other") == test.Print());
	}

	WHEN("A Lua object is created with a Lua object")
	{
		state.doString("test = TestClass()");
		state.doString("other = OtherClass(test)");
		state.doString("result = test:Print() == other:Print()");

		REQUIRE(state.getGlobal<bool>("result"));
	}
}
