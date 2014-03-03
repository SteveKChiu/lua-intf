//
// https://github.com/SteveKChiu/lua-intf
//
// Copyright 2014, Steve K. Chiu <steve.k.chiu@gmail.com>
//
// The MIT License (http://www.opensource.org/licenses/mit-license.php)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

template <typename T>
struct CppBindVariable
{
    /**
     * lua_CFunction to get a variable.
     *
     * This is used for global variables or class static data members.
     *
     * The pointer to the data is in the first upvalue.
     */
    static int get(lua_State* L)
    {
        try {
            assert(lua_islightuserdata(L, lua_upvalueindex(1)));

            auto ptr = static_cast<const T*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(ptr);

            LuaType<T>::push(L, *ptr);
            return 1;
        } catch (std::exception& e) {
            return luaL_error(L, e.what());
        }
    }

    /**
     * lua_CFunction to set a variable.
     *
     * This is used for global variables or class static data members.
     *
     * The pointer to the data is in the first upvalue.
     */
    static int set(lua_State* L)
    {
        try {
            assert(lua_islightuserdata(L, lua_upvalueindex(1)));

            auto ptr = static_cast<T*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(ptr);

            *ptr = LuaType<T>::get(L, 1);
            return 0;
        } catch (std::exception& e) {
            return luaL_error(L, e.what());
        }
    }
};

//----------------------------------------------------------------------------

template <typename FN, int IARG, typename R, typename... P>
struct CppBindMethodBase
{
    /**
     * lua_CFunction to call a function
     *
     * The pointer to function object is in the first upvalue.
     */
    static int call(lua_State* L)
    {
        try {
            assert(lua_isuserdata(L, lua_upvalueindex(1)));

            const FN& fn = *reinterpret_cast<const FN*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fn);

            typename CppArgTuple<P...>::Type args;
            CppArgInput<P...>::get(L, IARG, args);

            int n = CppInvokeMethod<FN, R, typename CppArg<P>::ValueType...>::push(L, fn, args);
            return n + CppArgOutput<P...>::push(L, args);
        } catch (std::exception& e) {
            return luaL_error(L, e.what());
        }
    }
};

template <typename FN, typename ARGS = FN, int IARG = 1>
struct CppBindMethod;

template <typename R, typename... P, int IARG>
struct CppBindMethod <R(*)(P...), R(*)(P...), IARG>
    : CppBindMethodBase <R(*)(P...), IARG, R, P...> {};

template <typename R, typename... P, int IARG>
struct CppBindMethod <std::function<R(P...)>, std::function<R(P...)>, IARG>
    : CppBindMethodBase <std::function<R(P...)>, IARG, R, P...> {};

template <typename R, typename... A, typename... P, int IARG>
struct CppBindMethod <R(*)(A...), _arg(*)(P...), IARG>
    : CppBindMethodBase <R(*)(A...), IARG, R, P...>
{
    static_assert(sizeof...(A) == sizeof...(P), "the number of arguments and argument-specs do not match");
};

template <typename R, typename... A, typename... P, int IARG>
struct CppBindMethod <std::function<R(A...)>, _arg(*)(P...), IARG>
    : CppBindMethodBase <std::function<R(A...)>, IARG, R, P...>
{
    static_assert(sizeof...(A) == sizeof...(P), "the number of arguments and argument-specs do not match");
};

//----------------------------------------------------------------------------

struct CppBindModuleMetaMethod
{
    /**
     * __index metamethod for module members.
     *
     * Retrieving global functions methods, stored in the metatable.
     * Reading global variable and properties, stored in the ___getters table.
     */
    static int index(lua_State* L);

    /**
     * __newindex metamethod for module members.
     *
     * The ___setters table stores proxy functions for assignment to
     * global variable and properties
     */
    static int newIndex(lua_State* L);

    /**
     * Forward __call to sub class or module
     * The name of sub class/module is in upvalue(1)
     */
    static int forwardCall(lua_State* L);

    /**
     * lua_CFunction to report an error writing to a read-only value.
     *
     * The name of the variable is in the first upvalue.
     */
    static int errorReadOnly(lua_State* L);
};

//----------------------------------------------------------------------------

template <typename T>
class CppBindClass;

/**
 * Provides C++ to Lua registration capabilities.
 *
 * This class is not instantiated directly, call LuaBinding(L) to start
 * the registration process.
 */
class CppBindModule
{
    friend class CppBindClassBase;

private:
    explicit CppBindModule(const LuaRef& module)
        : m_meta(module)
    {
        m_meta.checkTable();
    }

    static std::string getFullName(const LuaRef& module, const char* name);

    void setGetter(const char* name, const LuaRef& getter);
    void setSetter(const char* name, const LuaRef& setter);
    void setReadOnly(const char* name);

public:
    /**
     * Copy constructor
     */
    CppBindModule(const CppBindModule& that)
        : m_meta(that.m_meta)
        {}

    /**
     * Move constructor for temporaries
     */
    CppBindModule(CppBindModule&& that)
        : m_meta(std::move(that.m_meta))
        {}

    /**
     * Copy assignment
     */
    CppBindModule& operator = (const CppBindModule& that)
    {
        m_meta = that.m_meta;
        return *this;
    }

    /**
     * Move assignment for temporaries
     */
    CppBindModule& operator = (CppBindModule&& that)
    {
        m_meta = std::move(that.m_meta);
        return *this;
    }

    /**
     * Open the global CppBindModule.
     */
    static CppBindModule bind(lua_State* L);

    /**
     * The underlying lua state
     */
    lua_State* L() const
    {
        return m_meta.state();
    }

    /**
     * The underlying meta table
     */
    LuaRef meta() const
    {
        return m_meta;
    }

    /**
     * Open a new or existing CppBindModule for registrations.
     */
    CppBindModule beginModule(const char* name);

    /**
     * Continue CppBindModule registration in the parent.
     *
     * Do not use this on the global CppBindModule.
     */
    CppBindModule endModule();

    /**
     * Add or replace a variable.
     */
    template <typename V>
    CppBindModule& addVariable(const char* name, V* v, bool writable = true)
    {
        setGetter(name, LuaRef::createFunctionWithPtr(L(), &CppBindVariable<V>::get, v));
        if (writable) {
            setSetter(name, LuaRef::createFunctionWithPtr(L(), &CppBindVariable<V>::set, v));
        } else {
            setReadOnly(name);
        }
        return *this;
    }

    /**
     * Add or replace a static property member.
     *
     * If the set function is null, the property is read-only.
     */
    template <typename TG, typename TS>
    CppBindModule& addProperty(const char* name, TG (*get)(), void (*set)(TS) = nullptr)
    {
        setGetter(name, LuaRef::createFunction(L(), &CppBindMethod<TG(*)()>::call, get));
        if (set) {
            setSetter(name, LuaRef::createFunction(L(), &CppBindMethod<void(*)(TS)>::call, set));
        } else {
            setReadOnly(name);
        }
        return *this;
    }

    /**
     * Add or replace a read-write property.
     */
    template <typename T>
    CppBindModule& addProperty(const char* name, const std::function<T()>& get, const std::function<void(const T&)>& set)
    {
        setGetter(name, LuaRef::createFunction(L(), &CppBindMethod<std::function<T()>>::call, get));
        setSetter(name, LuaRef::createFunction(L(), &CppBindMethod<std::function<void(const T&)>>::call, set));
        return *this;
    }

    /**
     * Add or replace a read-only property.
     */
    template <typename T>
    CppBindModule& addProperty(const char* name, const std::function<T()>& get)
    {
        setGetter(name, LuaRef::createFunction(L(), &CppBindMethod<std::function<T()>>::call, get));
        setReadOnly(name);
        return *this;
    }

    /**
     * Add or replace a function.
     */
    template <typename FN>
    CppBindModule& addFunction(const char* name, const FN& proc)
    {
        static_assert(!std::is_function<FN>::value, "function pointer is needed, please prepend & to function name");
        m_meta.rawset(name, LuaRef::createFunction(L(), &CppBindMethod<FN>::call, proc));
        return *this;
    }

    /**
     * Add or replace a function, user can specify augument spec.
     *
     */
    template <typename FN, typename ARGS>
    CppBindModule& addFunction(const char* name, const FN& proc, ARGS)
    {
        static_assert(!std::is_function<FN>::value, "function pointer is needed, please prepend & to function name");
        m_meta.rawset(name, LuaRef::createFunction(L(), &CppBindMethod<FN, ARGS>::call, proc));
        return *this;
    }

    /**
     * Add or replace a factory function, that use lua "__call"
     */
    template <typename FN>
    CppBindModule& addFactory(const FN& proc)
    {
        static_assert(!std::is_function<FN>::value, "function pointer is needed, please prepend & to function name");
        m_meta.rawset("__call", LuaRef::createFunction(L(), &CppBindMethod<FN, FN, 2>::call, proc));
        return *this;
    }

    /**
     * Add or replace a factory function, that use lua "__call", user can specify augument spec.
     */
    template <typename FN, typename ARGS>
    CppBindModule& addFactory(const FN& proc, ARGS)
    {
        static_assert(!std::is_function<FN>::value, "function pointer is needed, please prepend & to function name");
        m_meta.rawset("__call", LuaRef::createFunction(L(), &CppBindMethod<FN, ARGS, 2>::call, proc));
        return *this;
    }

    /**
     * Add or replace a factory function, that forward "__call" to sub module factory (or class constructor)
     */
    CppBindModule& addFactory(const char* name)
    {
        m_meta.rawset("__call", LuaRef::createFunctionWithArgs(L(), &CppBindModuleMetaMethod::forwardCall, name));
        return *this;
    }

    /**
     * Open a new or existing class for registrations.
     */
    template <typename T>
    CppBindClass<T> beginClass(const char* name)
    {
        return CppBindClass<T>::bind(m_meta, name);
    }

    /**
     * Open a new class to extend the base class.
     */
    template <typename T, typename SUPER>
    CppBindClass<T> beginExtendClass(const char* name)
    {
        return CppBindClass<T>::extend(m_meta, name, CppObjectType<SUPER>::staticID());
    }

private:
    LuaRef m_meta;
};

//---------------------------------------------------------------------------

/**
 * Retrieve the root CppBindModule.
 *
 * It is recommended to put your module inside the global, and
 * then add your classes and functions to it, rather than adding many classes
 * and functions directly to the global
 */
inline CppBindModule LuaBinding(lua_State* L)
{
    return CppBindModule::bind(L);
}
