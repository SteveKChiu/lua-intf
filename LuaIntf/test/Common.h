#ifndef LUAINTF_COMMON_TEST
#define LUAINTF_COMMON_TEST

#include "LuaIntf/LuaIntf.h"
#include "catch.hpp"

using namespace LuaIntf;

namespace LuaIntf
{
	LUA_USING_SHARED_PTR_TYPE(std::shared_ptr)
}

struct TestClass {
	int number; ///< Set as a member variable
	std::string string; ///< Set as a member property using lambda getters/setters
	float other; ///< Set as a member property using defined getters/setters
	const int const_int = 8; ///< Set as member variable

	static int static_number; ///< Set as a static variable
	static std::string static_string; ///< Set as a static property using lambda getters/setters
	static float static_other; ///< Set as a static property using defined getters/setters
	static const int static_const; ///< Set as a static variable

	TestClass(int num = 0, std::string text = "default"): number(num), string(text), other(0.5f)
	{
		++instance_counter;
	}

	TestClass(TestClass&& that)
	{
		number = std::move(that.number);
		string = std::move(that.string);
		other = std::move(that.other);
    	++instance_counter;
	}

	TestClass(const TestClass& that)
	{
		number = that.number;
		string = that.string;
		other = that.other;
		++instance_counter;
	}

	~TestClass()
	{
		--instance_counter;
	}

	TestClass& operator=(const TestClass& other) = delete;
	TestClass& operator=(TestClass&& other) = delete;


	float GetFloat() const
	{
		return other;
	}

	void SetFloat(float value)
	{
		other = value;
	}

	static float GetStatic()
	{
		return static_other;
	}

	static void SetStatic(float value)
	{
		static_other = value;
	}

	/// Member Function
	std::string Print() const
	{
		return "{" + std::to_string(number) + ", " + string + ", " + std::to_string(other) + "}";
	}

	/// Static Function
	static int GetCount()
	{
		return instance_counter;
	}

	static void Register(lua_State* state)
	{
		LuaBinding(state).beginClass<TestClass>("TestClass")
			.addConstructor(LUA_ARGS(_opt<int>, _opt<std::string>))
			.addVariable("number", &TestClass::number)
			.addProperty("string", [](const TestClass* obj) { return obj->string; }, [](TestClass* obj, const std::string& text){ obj->string = text; })
			.addProperty("float", &TestClass::GetFloat, &TestClass::SetFloat)
			.addVariable("const", &TestClass::const_int)
			.addFunction("Print", &TestClass::Print)

			.addStaticVariable("number", &TestClass::static_number)
			.addStaticProperty("string", [](){ return TestClass::static_string; }, [](const std::string& text){ TestClass::static_string = text; })
			.addStaticProperty("float", &TestClass::GetStatic, &TestClass::SetStatic)
			.addStaticVariable("const", &TestClass::static_const)
			.addStaticFunction("GetCount", &TestClass::GetCount)
		.endClass();
	}

	private:
		static int instance_counter; ///< Used for GC tests, has only a static getter.
};

#endif // LUAINTF_COMMON_TEST
