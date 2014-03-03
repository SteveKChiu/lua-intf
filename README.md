lua-intf
========

LuaIntf is an interface binding between C++11 and Lua language, it provides three different set of API in one package:

1. low level API as simple wrapper for lua C API

This is a simple wrapper of one-to-one mapping for Lua C API, consider the following code:

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

2. high level API to access Lua object

`LuaRef` is designed to provide easy access to Lua object, and in most cast you don't have to deal with lua stack like the low level API. For example, the above code can be rewritten as:

	LuaRef func = LuaRef::fromGlobal("my_method");
	func(1, "yes", true);

Table access is as simple:

	LuaRef table = LuaRef::fromGlobal("my_table");
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

3. export C++ class/function to Lua script

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

		.addFunction<std::function(std::string())>("lambda", [] {
			// you can use C++11 lambda expression here too
			return "yes";
		})

	.endClass();

To access the exported `Web` class in lua:

	local w = web()								// auto w = Web("");
	w.url = "http://www.yahoo.com"				// w.set_url("http://www.yahoo.com");
	local page = w:load()						// auto page = w.load("");
	page = w:load("http://www.google.com")		// page = w.load("http://www.google.com");
	local url = w.url							// auto url = w.url();

LuaIntf is inspired by [vinniefalco's LuaBridge](https://github.com/vinniefalco/LuaBridge) work, but has been rewritten to take advantage of C++11 features.

