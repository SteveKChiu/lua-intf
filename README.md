lua-intf
========

`lua-intf` is a binding between C++11 and Lua language, it provides three different set of API in one package:

+ `LuaBinding`, Export C++ class or function to Lua script
+ `LuaRef`, High level API to access Lua object
+ `LuaState`, Low level API as simple wrapper for Lua C API

`lua-intf` has no dependencies other than Lua and C++11. And by default it is headers-only library, so there is no makefile or other install instruction, just copy the source, include `LuaIntf.h`, and you are ready to go.

`lua-intf` is inspired by [vinniefalco's LuaBridge](https://github.com/vinniefalco/LuaBridge) work, but has been rewritten to take advantage of C++11 features.


Export C++ class or function to Lua script
------------------------------------------

You can easily export C++ class or function for Lua script, consider the following C++ class:

	class Web
	{
	public:
		// base_url is optional
		Web(const std::string& base_url);
		~Web();

		static void go_home();

		static std::string home_url();
		static void set_home_url(const std::string& url);

		std::string url() const;
		void set_url(const std::string& url);
		std::string resolve_url(const std::string& uri);

		// doing reload if uri is empty
		std::string load(const std::string& uri);
	};

You can export the `Web` class by the following code:

    LuaBinding(L).beginClass<Web>("Web")
		.addConstructor(LUA_ARGS(_opt<std::string>))
		.addStaticProperty("home_url", &Web::home_url, &Web::set_home_url)
		.addStaticFunction("go_home", &Web::go_home)
		.addProperty("url", &Web::url, &Web::set_url)
		.addFunction("resolve_url", &Web::resolve_url)
		.addFunction("load", &Web::load, LUA_ARGS(_opt<std::string>))
		.addStaticFunction<std::function<std::string()>>("lambda", [] {
			// you can use C++11 lambda expression here too
			return "yes";
		})
	.endClass();

To access the exported `Web` class in Lua:

	local w = Web()								-- auto w = Web("");
	w.url = "http://www.yahoo.com"				-- w.set_url("http://www.yahoo.com");
	local page = w:load()						-- auto page = w.load("");
	page = w:load("http://www.google.com")		-- page = w.load("http://www.google.com");
	local url = w.url							-- auto url = w.url();

Module and class
----------------

C++ class or functions exported are organized by module and class. The general layout of binding looks like:

	LuaBinding(L)
		.beginModule(string module_name)
			.addFactory(function* func)
			.addVariable(string property_name, VARIABLE_TYPE* var, bool writable = true)
			.addProperty(string property_name, function* getter, function* setter)
			.addFunction(string function_name, function* func)

			.beginModule(string sub_module_name)
				...
			.endModule()

			.beginClass<CXX_TYPE>(string class_name)
				...
			.endClass()

			.beginExtendClass<CXX_TYPE, SUPER_CXX_TYPE>(string sub_class_name)
				...
			.endClass()
		.endModule()

		.beginClass<CXX_TYPE>(string class_name)
			.addFactory(FUNCTION_TYPE func)		// you can only have one addFactory or addConstructor
			.addConstructor(LUA_ARGS(...))
			.addStaticVariable(string property_name, VARIABLE_TYPE* var, bool writable = true)
			.addStaticProperty(string property_name, FUNCTION_TYPE getter, FUNCTION_TYPE setter)
			.addStaticFunction(string function_name, FUNCTION_TYPE func)
			.addVariable(string property_name, CXX_TYPE::FIELD_TYPE* var, bool writable = true)
			.addProperty(string property_name, CXX_TYPE::FUNCTION_TYPE getter, CXX_TYPE::FUNCTION_TYPE setter)
			.addFunction(string function_name, CXX_TYPE::FUNCTION_TYPE func)
		.endClass()

		.beginExtendClass<CXX_TYPE, SUPER_CXX_TYPE>(string sub_class_name)
			...
		.endClass()

A module binding is like a package or namespace, it can also contain other sub-module or class. A module can have the following bindings:

+ factory function - bind to global or static C++ functions, or forward to sub-class constructor or sub-module factory
+ property - bind to global or static variable, or getter and setter functions, property can be read-only
+ function - bind to global or static function

A class binding is modeled after C++ class, it models the const-ness correctly, so const object can not access non-const functions. It can have the following bindings:

+ constructor - bind to class constructor
+ factory function - bind to global or static function that return the class object
+ static property - bind to global or static variable, or getter and setter functions, property can be read-only
+ static function - bind to global or static function
+ member property - bind to member fields, or member getter and setter functions, property can be read-only
+ member function - bind to member functions

For module and class, you can only have one constructor or factory function. To access the factory or constructor in Lua script, you call the module or class name like a function, for example the above `Web` class:

	local w = Web("http://www.google.com")

The static or module property is accessible by module/class name, it is just like table field:

	local url = Web.home_url
	Web.home_url = "http://www.google.com"

The static function can be called by the following, note the '.' syntax and class name:

	Web.go_home()

The member property is associated with object, so it is accessible by variable:

	local session = Web("http://www.google.com")
	local url = session.url
	session.url = "http://www.google.com"

The member function can be called by the following, note the ':' syntax and object name:

	session:load("http://www.yahoo.com")

C++ object life-cycle
---------------------

`lua-intf` store C++ object via Lua `userdata`, and it stores the object in the following ways:

+ By value, the C++ object is stored inside `userdata`. So when Lua need to gc the `userdata`, the memory is automatically released. `lua-intf` will make sure the C++ destructor is called when that happened. C++ constructor or function returns value will create this kind of Lua object.

+ By pointer, only the pointer is stored inside `userdata`. So when Lua need to gc the `userdata`, the object is still alive. The object is owned by C++ code, it is important the registered function do not return pointer to newly allocated object that need to be deleted explicitly, otherwise there will be memory leak. C++ function returns pointer or reference will create this kind of Lua object.

+ By shared pointer, the shared pointer is stored inside `userdata`. So when Lua need to gc the `userdata`, the shared pointer is destructed, that usually means Lua is done with the object. If the object is still referenced by other shared pointer, it will keep alive, otherwise it will be deleted as expected. C++ function returns shared pointer will create this kind of Lua object. A special version of `addConstructor` will also create shared pointer automatically.

Using shared pointer
--------------------

If both C++ and Lua code need to access the same object, it is usually better to use shared pointer in both side, thus avoiding memory leak.

You can use any kind of shared pointer class, as long as it overrides `operator ->`, `operator *`, `operator bool` and have default and copy constructor. Before you can use it, you need to register it with `lua-intf`:

	namespace LuaIntf
	{
		LUA_USING_SHARED_PTR_TYPE(std::shared_ptr)
	}

For constructing shared pointer inside Lua `userdata`, you can register the constructor by adding LUA_SP macro and the actual shared pointer type, for example:

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

	// allow arg1, arg2 to map to arguments
	int func_2(lua_state* L, const std::string& arg1, int arg2);

	// this is *NOT* CFunction but normal function
	// the L can be placed anywhere, and it is stub to capture lua_State*,
	// and do not contribute to actual Lua arguments
	int func_3(const std::string& arg1, lua_state* L);

	class Object
	{
	public:
		// class method can follow CFunction convention too
		int func_1(lua_state* L);

		// allow arg1, arg2 to map to arguments
		int func_2(lua_state* L, const std::string& arg1, int arg2);
	};

	// the following can also be CFunction convention if it is added as class functions
	// note the first argument must be the pointer type of the registered class
	int obj_func_1(Object* obj, lua_state* L);
	int obj_func_2(Object* obj, lua_state* L, const std::string& arg1, int arg2);

For every function registration, `lua-intf` also support C++11 `std::function<>` type, so you can use `std::bind` or lambda expression if needed. Note you have to declare the function type with `std::function<>`

    LuaBinding(L).beginClass<Web>("Web")
		.addStaticFunction<std::function<std::string()>>("lambda", [] {
			// you can use C++11 lambda expression here too
			return "yes";
		})
		.addStaticFunction<std::function<std::string()>>("bind",
			std::bind(&Web::url, other_web_object))
	.endClass();

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

	static std::string match(const std::string& src, const std::string& pat, int pos, int& found_pos);

	LuaBinding(L).beginModule("utils")

		// this will return (string) (found_pos)
		.addFunction("match", &match, LUA_ARGS(std::string, std::string, _def<int, 1>, _out<int&>))

	.endModule();

Yet another way to return multiple results is to use `std::tuple<>`:

	static std::tuple<std::string, int> match(const std::string& src, const std::string& pat, int pos);

	LuaBinding(L).beginModule("utils")

		// this will return (string) (found_pos)
		.addFunction("match", &match)

	.endModule();

And the `std::tuple<>` works with `LuaRef` too, note the following code works with either definition (`_out<int>` or `std::tuple<>`) or the normal Lua function:

	LuaRef func(L, "utils.match");
    std::string found;
    int found_pos;
    std::tie(found, found_pos) = func.call<std::tuple<std::string, int>>("this is test", "test");

If your C++ function is overloaded, pass `&funcion` is not enough, you have to explicitly cast it to proper type:

	static int test(string, int);
	static string test(string);

	LuaBinding(L).beginModule("utils")

		// this will bind int test(string, int)
		.addFunction("test_1", static_cast<int(*)(string, int)>(&test))

		// this will bind string test(string), by using our LUA_FN macro
		.addFunction("test_2", LUA_FN(string, test, string))

	.endModule();

Custom type mapping
-------------------

It is possible to add primitive type mapping to the `lua-intf`, all you need to do is to add template specialization to LuaValueType. You need to define `ValueType` type, `void push(lua_State* L, const ValueType& v)`, `ValueType get(lua_State* L, int index)` and `ValueType opt(lua_State* L, int index, const ValueType& def)` functions. For example, to add Qt `QString` mapping to Lua string:

	namespace LuaIntf
	{

	template <>
	struct LuaValueType <QString>
	{
	    typedef QString ValueType;

	    static void push(lua_State* L, const ValueType& str)
	    {
	        if (str.isEmpty()) {
	            lua_pushliteral(L, "");
	        } else {
	            QByteArray buf = str.toUtf8();
	            lua_pushlstring(L, buf.data(), buf.length());
	        }
	    }

	    static ValueType get(lua_State* L, int index)
	    {
	        size_t len;
	        const char* p = luaL_checklstring(L, index, &len);
	        return QString::fromUtf8(p, len);
	    }

	    static ValueType opt(lua_State* L, int index, const ValueType& def)
	    {
	        if (lua_isnoneornil(L, index)) {
	            return def;
	        } else {
	            size_t len;
	            const char* p = luaL_checklstring(L, index, &len);
	            return QString::fromUtf8(p, len);
	        }
	    }
	};

	LUA_USING_VALUE_TYPE(QString)

	} // namespace LuaIntf

After that, you are able to push QString onto or get QString from Lua stack directly:

	QString s = ...;
	Lua::push(L, s);

	...

	s = Lua::pop<QString>(L);


High level API to access Lua object
-----------------------------------

`LuaRef` is designed to provide easy access to Lua object, and in most case you don't have to deal with Lua stack like the low level API. For example:

	lua_State* L = ...;
    lua_getglobal(L, "module");
    lua_getfield(L, -1, "method");
    lua_pushintteger(L, 1);
    lua_pushstring(L, "yes");
    lua_pushboolean(L, true);
    lua_call(L, 3, 0);

The above code can be rewritten as:

	LuaRef func(L, "module.method");
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

	lua_State* L = ...;
	LuaRef v = ...;
    lua_getglobal(L, "my_method");
    Lua::push(L, 1);					// the same as lua_pushinteger
    Lua::push(L, v);					// push v to lua stack
    Lua::push(L, true);					// the same as lua_pushboolean
    lua_call(L, 3, 2);
	LuaRef r(L, -2); 					// map r to lua stack index -2

Low level API as simple wrapper for Lua C API
---------------------------------------------

`LuaState` is a simple wrapper of one-to-one mapping for Lua C API, consider the following code:

	lua_State* L = ...;
    lua_getglobal(L, "module");
    lua_getfield(L, -1, "method");
    lua_pushintteger(L, 1);
    lua_pushstring(L, "yes");
    lua_pushboolean(L, true);
    lua_call(L, 3, 0);

It can be effectively rewritten as:

	LuaState lua = L;
    lua.getGlobal("module");
    lua.getField(-1, "method");
	lua.push(1);
	lua.push("yes");
	lua.push(true);
	lua.call(3, 0);

This low level API is completely optional, and you can still use the C API, or mix the usage. `LuaState` is designed to be a lightweight wrapper, and has very little overhead (if not as fast as the C API), and mostly can be auto-casting to or from `lua_State*`. In the `lua-intf`, `LuaState` and `lua_State*` are inter-changeable, you can pick the coding style you like most.

`LuaState` does not manage `lua_State*` life-cycle, you may take a look at `LuaContext` class for that purpose.
