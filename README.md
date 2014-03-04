lua-intf
========

`lua-intf` is a binding between C++11 and Lua language, it provides three different set of API in one package:

+ `LuaState`, Low level API as simple wrapper for Lua C API
+ `LuaRef`, High level API to access Lua object
+ `LuaBinding`, Export C++ class or function to Lua script

`lua-intf` has no other dependencies other than Lua and C++11. And by default it is headers-only library, so there is no makefile or other install instruction, just copy the source, include `LuaIntf.h`, and you are ready to go.

`lua-intf` is inspired by [vinniefalco's LuaBridge](https://github.com/vinniefalco/LuaBridge) work, but has been rewritten to take advantage of C++11 features.

Low level API as simple wrapper for Lua C API
---------------------------------------------

`LuaState` is a simple wrapper of one-to-one mapping for Lua C API, consider the following code:

	lua_State* L = ...;
    lua_getglobal(L, "my_method");
    lua_pushintteger(L, 1);
    lua_pushstring(L, "yes");
    lua_pushboolean(L, true);
    lua_call(L, 3, 0);

It can be effectively rewritten as:

	LuaState lua = L;
    lua_getglobal(L, "my_method");
	lua.push(1);
	lua.push("yes");
	lua.push(true);
	lua.call(3, 0);

This low level API is completely optional, and you can still use the C API, or mix the usage. `LuaState` is designed to be a lightweight wrapper, and has very little overhead (if not as fast as the C API), and mostly can be auto-casting to or from `lua_State` pointer.

`LuaState` does not manage `lua_State` life-cycle, you may take a look at `LuaContext` class for that purpose.

High level API to access Lua object
-----------------------------------

`LuaRef` is designed to provide easy access to Lua object, and in most cast you don't have to deal with Lua stack like the low level API. For example, the above code can be rewritten as:

	LuaRef func(L, "my_method");
	func(1, "yes", true);

Table access is as simple:

	LuaRef table(L, "my_table");
	table["value"] = 15;
	int value = table.get<int>("value");

	for (auto& e : table) {
		std::string key = e.key<std::string>();
		LuaRef value = e.value<LuaRef>();
		...
	}

And you can mix it with the low level API:

	LuaState lua = L;
	LuaRef v = ...;
	lua.getGlobal("my_method");
	lua.push(1);
	lua.push(v); 						// push v to lua stack
	lua.push(true);
	lua.call(3, 1);
	LuaRef r = LuaRef::popFromStack(L); // pop value from lua stack

For C API lover, you can use the `Lua` helper too:

	lua_State* L = ...;
	LuaRef v = ...;
    lua_getglobal(L, "my_method");
    Lua::push(L, 1);
    Lua::push(L, v);					// push v to lua stack
    Lua::push(L, true);
    lua_call(L, 3, 2);
	LuaRef r(L, -2); 					// map r to lua stack index -2

Export C++ class or function to Lua script
------------------------------------------

You can easily export C++ class or function for lua, consider the following C++ class:

	class Web
	{
	public:
		// base_url is optional
		Web(const std::string& base_url);
		~Web();

		std::string url() const;
		void set_url(const std::string& url);
		std::string resolve_url(const std::string& uri);

		// doing reload if uri is empty
		std::string load(const std::string& uri);
	};

You can export the `Web` class by the following code:

    LuaBinding(L).beginClass<Web>("web")
		.addConstructor(LUA_ARGS(_opt<std::string>))
		.addProperty("url", &Web::url, &Web::set_url)
		.addFunction("resolve_url", &Web::resolve_url)
		.addFunction("load", &Web::load, LUA_ARGS(_opt<std::string>))
		.addFunction<std::function<std::string()>>("lambda", [] {
			// you can use C++11 lambda expression here too
			return "yes";
		})
	.endClass();

To access the exported `Web` class in Lua:

	local w = web()								-- auto w = Web("");
	w.url = "http://www.yahoo.com"				-- w.set_url("http://www.yahoo.com");
	local page = w:load()						-- auto page = w.load("");
	page = w:load("http://www.google.com")		-- page = w.load("http://www.google.com");
	local url = w.url							-- auto url = w.url();

C++ object lifecycle
--------------------

`lua-intf` store C++ object via Lua `userdata`, and it stores the object in the following ways:

+ By value, the C++ object is stored inside `userdata`. So when Lua need to gc the `userdata`, the memory is automatically released. `lua-intf` will make sure the C++ destructor is called when that happend. C++ constructor or function returns value will create this kind of Lua object.

+ By pointer, only the pointer is stored inside `userdata`. So when Lua need to gc the `userdata`, the object is still alive. The object is owned by C++ code, it is important you do not pass newly allocated object that need to be deleted explicitly via pointer, otherwise there will be memory leak. C++ function returns pointer or reference will create this kind of Lua obejct.

+ By shared pointer, the shared pointer is stored inside `userdata`. So when Lua need to gc the `userdata`, the shared pointer is destructed, that usually tell Lua is done with the object. If the object is referenced by other shared pointer, it will keep alive, otherwise it will de deleted as expected. C++ function returns shared pointer will create this kind of Lua obejct. A special version of `addConstructor` will also create shared pointer automatically.

Using shared pointer
--------------------

If both C++ and Lua code need to access the same object, it is usually better to use shared pointer in both side, thus avoiding memory leak.

You can use any kind of shared pointer class, as long as it overrides `operator ->` and `operator *`. Before you can use it, you need to register it with `lua-intf`:

	namespace LuaIntf
	{
		LUA_USING_SHARED_PTR_TYPE(std::shared_ptr)
	}

For construting shared pointer inside Lua `userdata`, you can add the constructor by adding LUA_SP macro and the actual shared pointer type, for example:

    LuaBinding(L).beginClass<Web>("web")
		.addConstructor(LUA_SP(std::shared_ptr<Web>), LUA_ARGS(_opt<std::string>))
		...
	.endClass();

Function calling convention
---------------------------

C++ function exported to Lua can follow one the two calling conventions:

+ Normal function, that return value will be the value seen on the Lua side.

+ Lua `CFunction` convention, that the first argument is `lua_State*` and the return type is `int`. And just like `CFunction` you need to manually push the result onto Lua stack, and return the number of results pushed.

`lua-intf` extends `CFunction` convention by allowing more arguments besides `lua_State*`, the following functions will all follow the `CFunction` convention:

	int func_1(lua_state* L);
	int func_2(lua_state* L, const std::string& arg1, int arg2);	// allow arg1, arg2 to map to argiments
	int func_3(LuaState lua, const std::string& arg1, int arg2);	// lua_State* and LuaState are inter-changable

	// this is *NOT* CFunction but normal function
	// the L can be placed anywhere, and it is stub to capture lua_State*, and do not contribute to actual Lua arguments
	int funcs4(const std::string& arg1, lua_state* L);

	class Object
	{
	public:
		int func_1(lua_state* L);		// class method can follow CFunction convention too
		int func_2(lua_state* L, const std::string& arg1, int arg2);	// allow arg1, arg2 to map to argiments
		int func_3(LuaState lua, const std::string& arg1, int arg2);	// lua_State* and LuaState are inter-changable
	};

	// the follwoing can also be CFunction convention if it is added as class functions
	// note the first argument must be the pointer type of the registered class
	int obj_func_1(Object* obj, lua_state* L);
	int obj_func_2(Object* obj, lua_state* L, const std::string& arg1, int arg2);
	int obj_func_3(Object* obj, LuaState lua, const std::string& arg1, int arg2);

For every function registration, `lua-intf` also support C++ `std::function<>` type, so you can use `std::bind` or lambda expression if needed.

`lua-intf` make it possible to bind optional `_opt<ARG_TYPE>` or default arguments `_def<ARG_TYPE, DEF_VALUE>` for Lua. For example:

	struct MyString
	{
		std::string indexOf(const std::string& str, int pos);
		...
	};

	LuaBinding(L).beginClass<MyString>("mystring")

		// this will make pos = 1 if it is not specified in Lua side
		.addFunction("indexOf", &MyString::indexOf, LUA_ARGS(std::string, _def<int, 1>))

	.endClass();

It is also possible to return multiple results by telling which argument is for output `_out<ARG_TYPE>` or input-output `_ref<ARG_TYPE>` `_ref_opt<ARG_TYPE>` `_ref_def<ARG_TYPE, DEF_VALUE>`. For example:

	static std::string match(const std::string& src, const std::string& pat, int pos, int* found_pos);

	LuaBinding(L).beginModule("utils")

		// this will return (string) (found_pos)
		.addFunction("match", &match, LUA_ARGS(std::string, std::string, _def<int, 1>, _out<int>))

	.endModule();

Yet another way to return multiple results is to using `std::tuple<>`:

	static std::tuple<std::string, int> match(const std::string& src, const std::string& pat, int pos);

	LuaBinding(L).beginModule("utils")

		// this will return (string) (found_pos)
		.addFunction("match", &match)

	.endModule();

And the `std::tuple<>` works with `LuaRef` too, note the following code works with either definition (`_out<int>` or `std::tuple<>`):

	LuaRef func(L, "utils.match");
    std::string found;
    int found_pos;
    std::tie(found, found_pos) = func.call<std::tuple<std::string, int>>("this is test", "test");

