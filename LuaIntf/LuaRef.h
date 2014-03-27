//
// https://github.com/SteveKChiu/lua-intf
//
// Copyright 2013, Steve Chiu <steve.k.chiu@gmail.com>
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

#ifndef LUAREF_H
#define LUAREF_H

//---------------------------------------------------------------------------

#include "LuaState.h"
#include <functional>

namespace LuaIntf
{

class LuaRef;

//---------------------------------------------------------------------------

/**
 * Assignable and convertible result of bracket-style lookup
 */
class LuaTableRef
{
public:
    /**
     * Create new index association
     *
     * @param state the lua state
     * @param table the table reference (no auto unref)
     * @param key the table key reference (it will be unref automatically)
     */
    constexpr LuaTableRef(lua_State* state, int table, int key)
        : L(state)
        , m_table(table)
        , m_key(key)
        {}

    /**
     * Copy constructor
     */
    LuaTableRef(const LuaTableRef& that)
        : L(that.L)
        , m_table(that.m_table)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, that.m_key);
        m_key = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    /**
     * Move constructor
     */
    LuaTableRef(LuaTableRef&& that)
        : L(that.L)
        , m_table(that.m_table)
        , m_key(that.m_key)
    {
        that.m_key = LUA_NOREF;
    }

    /**
     * Destructor
     */
    ~LuaTableRef()
    {
        luaL_unref(L, LUA_REGISTRYINDEX, m_key);
    }

    /**
     * Copy value from other table[key] reference
     *
     * @param that reference from other table[key]
     */
    LuaTableRef& operator = (const LuaTableRef& that);

    /**
     * Assign value for table[key]
     *
     * @param value new value for table[key]
     * @throw LuaException if V are not convertible to Lua types
     */
    template <typename V>
    LuaTableRef& operator = (const V& value)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_table);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_key);
        Lua::push(L, value);
        lua_settable(L, -3);
        lua_pop(L, 1);
        return *this;
    }

    /**
     * Get value of table[key]
     *
     * @return value of t[k] as type V
     * @throw LuaException if V are not convertible to Lua types
     */
    template <typename V>
    V value() const
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_table);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_key);
        lua_gettable(L, -2);
        V v = Lua::get<V>(L, -1);
        lua_pop(L, 2);
        return v;
    }

private:
    lua_State* L;
    int m_table;
    int m_key;
};

//---------------------------------------------------------------------------

/**
 * C++ and java style const iterator for this table
 */
class LuaTableIterator
{
public:
    /**
     * Create empty LuaTableIterator, it must be assigned to other LuaTableIterator before using
     */
    constexpr LuaTableIterator()
        : L(nullptr)
        , m_table(LUA_NOREF)
        , m_key(LUA_NOREF)
        , m_value(LUA_NOREF)
        {}

    /**
     * Create LuaTableIterator for table
     *
     * @param state the lua state
     * @param table the lua table reference (no auto unref)
     * @param fetch_next true if fetch next entry first
     */
    LuaTableIterator(lua_State* state, int table, bool fetch_next);

    /**
     * Copy constructor
     */
    LuaTableIterator(const LuaTableIterator& that);

    /**
     * Move constructor
     */
    LuaTableIterator(LuaTableIterator&& that);

    /**
     * Destructor
     */
    ~LuaTableIterator();

    /**
     * Get entry (for loop inerator compatibility)
     */
    const LuaTableIterator& operator * () const
    {
        return *this;
    }

    /**
     * Advance to next entry
     */
    LuaTableIterator& operator ++ ()
    {
        next();
        return *this;
    }

    /**
     * Copy assignment
     */
    LuaTableIterator& operator = (const LuaTableIterator& that);

    /**
     * Move assignment
     */
    LuaTableIterator& operator = (LuaTableIterator&& that);

    /**
     * Test whether the two iterator is at same position
     */
    bool operator == (const LuaTableIterator& that) const;

    /**
     * Test whether the two iterator is at same position
     */
    bool operator != (const LuaTableIterator& that) const
    {
        return !operator == (that);
    }

    /**
     * Get the key of current entry
     */
    template <typename K>
    K key() const
    {
        if (!L) throw LuaException("invalid key reference");
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_key);
        return Lua::pop<K>(L);
    }

    /**
     * Get the value of current entry
     */
    template <typename V>
    V value() const
    {
        if (!L) throw LuaException("invalid value reference");
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_value);
        return Lua::pop<V>(L);
    }

private:
    void next();

private:
    lua_State* L;
    int m_table;
    int m_key;
    int m_value;
};

//---------------------------------------------------------------------------

/**
 * Lightweight reference to a Lua object.
 *
 *   The reference is maintained for the lifetime of the C++ object.
 */
class LuaRef
{
public:
    /**
     * Create new lua_CFunction with args as upvalue
     *
     * @param L lua state
     * @param proc the lua_CFunction
     * @param ref the object as upvalue(1)
     */
    template <typename... P>
    static LuaRef createFunctionWithArgs(lua_State* L, lua_CFunction proc, P&&... args)
    {
        pushArg(L, std::forward<P>(args)...);
        lua_pushcclosure(L, proc, sizeof...(P));
        return popFromStack(L);
    }

    /**
     * Create new lua_CFunction with allocated userdata as upvalue(1)
     *
     * @param L lua state
     * @param proc the lua_CFunction
     * @param userdata_size the size of userdata to allocate
     * @param out_userdata [out] the pointer to allocated userdata
     */
    static LuaRef createFunctionWithNewData(lua_State* L, lua_CFunction proc, size_t userdata_size, void** out_userdata)
    {
        void* userdata = lua_newuserdata(L, userdata_size);
        if (out_userdata) *out_userdata = userdata;
        lua_pushcclosure(L, proc, 1);
        return popFromStack(L);
    }

    /**
     * Create new lua_CFunction with allocated cpp object as upvalue(1)
     * The cpp object is constructed inside userdata (in place)
     */
    template <typename T, typename... P>
    static LuaRef createFunctionWithNewObj(lua_State* L, lua_CFunction proc, P&&... args)
    {
        static_assert(!std::is_function<T>::value, "function declaration is not allowed, use function pointer if needed");
        void* userdata;
        LuaRef f = createFunctionWithNewData(L, proc, sizeof(T), &userdata);
        new (userdata) T(std::forward<P>(args)...);
        return f;
    }

    /**
     * Create new lua_CFunction with context (lightuserdata) as upvalue(1)
     *
     * @param L lua state
     * @param proc the lua_CFunction
     * @param context the context pointer as upvalue(1)
     */
    static LuaRef createFunctionWithPtr(lua_State* L, lua_CFunction proc, void* ptr)
    {
        lua_pushlightuserdata(L, ptr);
        lua_pushcclosure(L, proc, 1);
        return popFromStack(L);
    }

    /**
     * Create new lua_CFunction with allocated object as upvalue(1)
     * The object is copied to userdata (using copy constructor)
     */
    template <typename T>
    static LuaRef createFunction(lua_State* L, lua_CFunction proc, const T& cpp)
    {
        static_assert(!std::is_function<T>::value, "function declaration is not allowed, use function pointer if needed");
        void* userdata;
        LuaRef f = createFunctionWithNewData(L, proc, sizeof(T), &userdata);
        new (userdata) T(cpp);
        return f;
    }

    /**
     * Call function on stack, and throw LuaException if failed
     *
     * @param L - the lua state
     * @param nargs - number of arguments on stack
     * @param nresult - number of result on stack
     */
    static void pcall(lua_State* L, int nargs, int nresult);

    /**
     * Create a new, empty table
     *
     * @param L lua state
     * @param narr pre-allocate space for this number of array elements
     * @param nrec pre-allocate space for this number of non-array elements
     * @return empty table
     */
    static LuaRef createTable(lua_State* L, int narr = 0, int nrec = 0)
    {
        lua_createtable(L, narr, nrec);
        return popFromStack(L);
    }

    /**
     * Get registry table
     */
    static LuaRef registry(lua_State* L)
    {
        return LuaRef(L, LUA_REGISTRYINDEX);
    }

    /**
     * Get global table (_G)
     */
    static LuaRef globals(lua_State* L)
    {
        lua_pushglobaltable(L);
        return popFromStack(L);
    }

    /**
     * Create empty reference
     */
    constexpr LuaRef()
        : L(nullptr)
        , m_ref(LUA_NOREF)
        {}

    /**
     * Create reference to object on stack, stack is not changed
     *
     * @param L lua state
     * @param index position on stack
     */
    explicit LuaRef(lua_State* state, int index = -1)
        : L(state)
    {
        if (!L) throw LuaException("invalid state");
        lua_pushvalue(L, index);
        m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    /**
     * Create reference to lua global
     *
     * @param L lua state
     * @param name the global name, may contains '.' to access sub obejct
     */
    LuaRef(lua_State* state, const char* name)
        : L(state)
    {
        if (!L) throw LuaException("invalid state");
        Lua::pushGlobal(L, name);
        m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    /**
     * Copy constructor, this is still reference the same Lua object
     */
    LuaRef(const LuaRef& that)
        : L(that.L)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, that.m_ref);
        m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    /**
     * Move constructor, this is still reference the same Lua object
     */
    LuaRef(LuaRef&& that)
        : L(that.L)
        , m_ref(that.m_ref)
    {
        that.m_ref = LUA_NOREF;
    }

    /**
     * Type conversion for LuaTableRef
     */
    LuaRef(const LuaTableRef& that)
        : LuaRef(that.value<LuaRef>())
        {}

    /**
     * Destructor, release the reference
     */
    virtual ~LuaRef()
    {
        if (L) {
            luaL_unref(L, LUA_REGISTRYINDEX, m_ref);
        }
    }

    /**
     * Copy assignment, this only copy the reference, so both reference the same object
     */
    LuaRef& operator = (const LuaRef& that);

    /**
     * Move assignment, this only apply to the reference
     */
    LuaRef& operator = (LuaRef&& that)
    {
        std::swap(L, that.L);
        std::swap(m_ref, that.m_ref);
        return *this;
    }

    /**
     * Get the underlying lua state
     */
    lua_State* state() const
    {
        return L;
    }

    /**
     * Check whether the reference is valid (bind to lua)
     */
    bool isValid() const
    {
        return L && m_ref != LUA_NOREF;
    }

    /**
     * Test whether this ref is table
     */
    bool isTable() const
    {
        return type() == LuaTypeID::Table;
    }

    /**
     * Test whether this ref is function
     */
    bool isFunction() const
    {
        return type() == LuaTypeID::Function;
    }

    /**
     * Get the lua type id
     */
    LuaTypeID type() const;

    /**
     * Get the lua type name
     */
    const char* typeName() const
    {
        pushToStack();
        const char* s = luaL_typename(L, -1);
        lua_pop(L, 1);
        return s;
    }

    /**
     * Check whether the reference is table
     * may throw LuaException if type is not matched
     */
    const LuaRef& checkTable() const
    {
        return checkType(LuaTypeID::Table);
    }

    /**
     * Check whether the reference is function
     * may throw LuaException if type is not matched
     */
    const LuaRef& checkFunction() const
    {
        return checkType(LuaTypeID::Function);
    }

    /**
     * Check whether the reference is in given type
     * may throw LuaException if type is not matched
     */
    const LuaRef& checkType(LuaTypeID type) const
    {
        pushToStack();
        luaL_checktype(L, -1, static_cast<int>(type));
        lua_pop(L, 1);
        return *this;
    }

    /**
     * Test whether this reference is identical to the other reference (not via any metatable)
     */
    bool isIdenticalTo(const LuaRef& r) const;

    /**
     * Compare this reference to the other reference
     * @return 0 if equal; -1 is less than; 1 if greater than
     */
    int compareTo(const LuaRef& r) const;

    /**
     * Test whether this reference is equal to the other reference
     */
    bool operator == (const LuaRef& r) const;

    /**
     * Test whether this reference is lua_nil
     */
    bool operator == (std::nullptr_t) const
    {
        return m_ref == LUA_REFNIL || m_ref == LUA_NOREF;
    }

    /**
     * Test whether this reference is not equal to the other reference
     */
    bool operator != (const LuaRef& r) const
    {
        return !operator == (r);
    }

    /**
     * Test whether this is not lua_nil
     */
    bool operator != (std::nullptr_t r) const
    {
        return !operator == (r);
    }

    /**
     * Test whether this reference is less than the other reference
     */
    bool operator < (const LuaRef& r) const;

    /**
     * Test whether this reference is less than or equal to the other reference
     */
    bool operator <= (const LuaRef& r) const;

    /**
     * Test whether this reference is greater than the other reference
     */
    bool operator > (const LuaRef& r) const
    {
        return !operator <= (r);
    }

    /**
     * Test whether this reference is greater than or equal to the other reference
     */
    bool operator >= (const LuaRef& r) const
    {
        return !operator < (r);
    }

    /**
     * Push value of this reference into lua stack
     */
    void pushToStack() const
    {
        if (!L) throw LuaException("invalid reference");
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_ref);
    }

    /**
     * Create LuaRef from top of lua stack, the top value is popped from stack
     */
    static LuaRef popFromStack(lua_State* L)
    {
        LuaRef r;
        r.L = L;
        r.m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return r;
    }

    /**
     * Cast to the given value type
     */
    template <typename T>
    T toValue() const
    {
        pushToStack();
        return Lua::pop<T>(L);
    }

    /**
     * Create LuaRef from value
     */
    template <typename T>
    static LuaRef fromValue(lua_State* L, const T& value)
    {
        Lua::push(L, value);
        return popFromStack(L);
    }

    /**
     * Cast to the data pointer, if this ref is not pointer, return nullptr
     */
    void* toPointer() const
    {
        pushToStack();
        void* ptr = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return ptr;
    }

    /**
     * Create LuaRef from data pointer, the data pointer life time is managed by C++
     */
    static LuaRef fromPointer(lua_State* L, void* ptr)
    {
        lua_pushlightuserdata(L, ptr);
        return popFromStack(L);
    }

    /**
     * Call this function
     *
     * @param args arguments to pass to function
     */
    template <typename... P>
    void operator () (P&&... args)
    {
        call<P...>(std::forward<P>(args)...);
    }

    /**
     * Call this function and without return value
     *
     * @param args arguments to pass to function
     */
    template <typename... P>
    void call(P&&... args)
    {
        lua_pushcfunction(L, &LuaException::traceback);
        pushToStack();
        pushArg(L, std::forward<P>(args)...);
        if (lua_pcall(L, sizeof...(P), 0, -int(sizeof...(P) + 2)) != LUA_OK) {
            lua_remove(L, -2);
            throw LuaException(L);
        }
        lua_pop(L, 1);
    }

    /**
     * Call this function and get one return value
     *
     * @param args arguments to pass to function
     * @return value of function
     */
    template <typename R, typename... P>
    R call(P&&... args)
    {
        lua_pushcfunction(L, &LuaException::traceback);
        pushToStack();
        pushArg(L, std::forward<P>(args)...);
        if (lua_pcall(L, sizeof...(P), 1, -int(sizeof...(P) + 2)) != LUA_OK) {
            lua_remove(L, -2);
            throw LuaException(L);
        }
        R v = Lua::get<R>(L, -1);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Call this function and get return value(s)
     * put types of expected return types in the template param
     *
     * std::tuple<int, int> ret = call<int, int>(arg1, arg2)
     *
     * @param args arguments to pass to function
     * @return values of function
     */
    template <typename... R, typename... P>
    typename std::enable_if<sizeof...(R) >= 2, std::tuple<R...>>::type
    call(P&&... args)
    {
        lua_pushcfunction(L, &LuaException::traceback);
        pushToStack();
        pushArg(L, std::forward<P>(args)...);
        if (lua_pcall(L, sizeof...(P), sizeof...(R), -int(sizeof...(P) + sizeof...(R) + 2)) != LUA_OK) {
            lua_remove(L, -2);
            throw LuaException(L);
        }
        std::tuple<R...> ret;
        Result<sizeof...(R), R...>::fill(L, ret);
        lua_pop(L, sizeof...(R) + 1);
        return ret;
    }

    /**
     * Get this table's metatable
     *
     * @return this table's metatable or nil if none
     */
    LuaRef getMetaTable() const;

    /**
     * Set this table's metatable
     *
     * @param meta new metatable
     */
    void setMetaTable(const LuaRef& meta);

    /**
     * Look up field in table in raw mode (not via metatable)
     *
     * @param key variable key
     * @return variable value
     * @throw LuaException if K or V are not convertible to Lua types
     */
    template<typename V, typename K>
    V rawget(const K& key) const
    {
        pushToStack();
        Lua::push(L, key);
        lua_rawget(L, -2);
        V v = Lua::get<V>(L, -1);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Look up field in table in raw mode (not via metatable)
     *
     * @param key variable key
     * @param def default value
     * @return variable value
     * @throw LuaException if K or V are not convertible to Lua types
     */
    template<typename V, typename K>
    V rawget(const K& key, const V& def) const
    {
        pushToStack();
        Lua::push(L, key);
        lua_rawget(L, -2);
        V v = Lua::opt<V>(L, -1, def);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Set field in table in raw mode (not via metatable)
     *
     * @param key variable key
     * @param value variable value
     * @throw LuaException if K or V are not convertible to Lua types
     */
    template <typename K, typename V>
    void rawset(const K& key, const V& value)
    {
        pushToStack();
        Lua::push(L, key);
        Lua::push(L, value);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    /**
     * Look up field in table in raw mode (not via metatable)
     *
     * @param key lightuserdata key
     * @return variable value
     * @throw LuaException if V are not convertible to Lua types
     */
    template<typename V>
    V rawget(void* p) const
    {
        pushToStack();
        lua_rawgetp(L, -1, p);
        V v = Lua::get<V>(L, -1);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Look up field in table in raw mode (not via metatable)
     *
     * @param key lightuserdata key
     * @param def default value
     * @return variable value
     * @throw LuaException if V are not convertible to Lua types
     */
    template<typename V>
    V rawget(void* p, const V& def) const
    {
        pushToStack();
        lua_rawgetp(L, -1, p);
        V v = Lua::opt<V>(L, -1, def);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Set field in table in raw mode (not via metatable)
     *
     * @param p lightuserdata key
     * @param value variable value
     * @throw LuaException if V are not convertible to Lua types
     */
    template <typename V>
    void rawset(void* p, const V& value)
    {
        pushToStack();
        Lua::push(L, value);
        lua_rawsetp(L, -2, p);
        lua_pop(L, 1);
    }

    /**
     * Look up field in table in raw mode (not via metatable)
     *
     * @param key lightuserdata key
     * @return variable value
     * @throw LuaException if V are not convertible to Lua types
     */
    template<typename V>
    V rawget(int i) const
    {
        pushToStack();
        lua_rawgeti(L, -1, i);
        V v = Lua::get<V>(L, -1);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Look up field in table in raw mode (not via metatable)
     *
     * @param key lightuserdata key
     * @param def default value
     * @return variable value
     * @throw LuaException if V are not convertible to Lua types
     */
    template<typename V>
    V rawget(int i, const V& def) const
    {
        pushToStack();
        lua_rawgeti(L, -1, i);
        V v = Lua::opt<V>(L, -1, def);
        lua_pop(L, 2);
        return v;
    }

    /**
     * Set field in table in raw mode (not via metatable)
     *
     * @param p lightuserdata key
     * @param value variable value
     * @throw LuaException if V are not convertible to Lua types
     */
    template <typename V>
    void rawset(int i, const V& value)
    {
        pushToStack();
        Lua::push(L, value);
        lua_rawseti(L, -2, i);
        lua_pop(L, 1);
    }

    /**
     * Get the length of this table (the same as # operator of lua, but not via metatable)
     */
    int rawlen() const
    {
        pushToStack();
        int n = lua_rawlen(L, -1);
        lua_pop(L, 1);
        return n;
    }

    /**
     * Test whether field is in table
     *
     * @param key variable key
     * @return true if field is available
     * @throw LuaException if K is not convertible to Lua types
     */
    template <typename K>
    bool has(const K& key) const
    {
        pushToStack();
        Lua::push(L, key);
        lua_gettable(L, -2);
        bool ok = !lua_isnoneornil(L, -1);
        lua_pop(L, 2);
        return ok;
    }

    /**
     * Look up field in table
     *
     * @param key variable key
     * @return variable value
     * @throw LuaException if K or V are not convertible to Lua types
     */
    template <typename V, typename K>
    V get(const K& key) const
    {
        pushToStack();
        Lua::push(L, key);
        lua_gettable(L, -2);
        V t = Lua::get<V>(L, -1);
        lua_pop(L, 2);
        return t;
    }

    /**
     * Look up field in table
     *
     * @param key variable key
     * @param def default value
     * @return variable value
     * @throw LuaException if K or V are not convertible to Lua types
     */
    template <typename V, typename K>
    V get(const K& key, const V& def) const
    {
        pushToStack();
        Lua::push(L, key);
        lua_gettable(L, -2);
        V t = Lua::opt<V>(L, -1, def);
        lua_pop(L, 2);
        return t;
    }

    /**
     * Set field in table
     *
     * @param key variable key
     * @param value variable value
     * @throw LuaException if K or V are not convertible to Lua types
     */
    template <typename K, typename V>
    void set(const K& key, const V& value)
    {
        pushToStack();
        Lua::push(L, key);
        Lua::push(L, value);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    /**
     * Remove field in table
     *
     * @param key variable key
     * @throw LuaException if K is not convertible to Lua types
     */
    template <typename K>
    void remove(const K& key)
    {
        pushToStack();
        Lua::push(L, key);
        lua_pushnil(L);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    /**
     * Get the length of this table (the same as # operator of lua)
     */
    int len() const
    {
        pushToStack();
        int n = luaL_len(L, -1);
        lua_pop(L, 1);
        return n;
    }

    /**
     * Get or set variable in table with bracket-style syntax
     *
     * @param key variable key
     * @return assignable and convertible handle for specified key in this table
     */
    template <typename K>
    LuaTableRef operator [] (const K& key)
    {
        Lua::push(L, key);
        return LuaTableRef(L, m_ref, luaL_ref(L, LUA_REGISTRYINDEX));
    }

    /**
     * Get variable in table with bracket-style syntax
     *
     * @param key variable key
     * @return convertible handle for specified key in this table
     */
    template <typename K>
    const LuaTableRef operator [] (const K& key) const
    {
        Lua::push(L, key);
        return LuaTableRef(L, m_ref, luaL_ref(L, LUA_REGISTRYINDEX));
    }

    /**
     * Get the C++ style const iterator
     */
    LuaTableIterator begin() const
    {
        return LuaTableIterator(L, m_ref, true);
    }

    /**
     * Get the C++ style const iterator
     */
    LuaTableIterator end() const
    {
        return LuaTableIterator(L, m_ref, false);
    }

private:
    template <typename T>
    static void fillResult(lua_State* L, int idx, T& r)
    {
        r = Lua::get<T>(L, idx, r);
    }

    template <size_t N, typename... P>
    struct Result
    {
        static void fill(lua_State* L, std::tuple<P...>& args)
        {
            fillResult(L, sizeof...(P) - N + 1, std::get<sizeof...(P) - N>(args));
            Result<N - 1, P...>::fill(L, args);
        }
    };

    template <typename... P>
    struct Result <0, P...>
    {
        static void fill(lua_State*, std::tuple<P...>&)
        {
            // template terminate function, do nothing
        }
    };

    template <typename P0, typename... P>
    static void pushArg(lua_State* L, P0&& p0, P&&... p)
    {
        Lua::push(L, std::forward<P0>(p0));
        pushArg(L, std::forward<P>(p)...);
    }

    static void pushArg(lua_State* L)
    {
        // template terminate function, do nothing
    }

private:
    lua_State* L;
    int m_ref;
};

//---------------------------------------------------------------------------

struct LuaRefType
{
    typedef LuaRef ValueType;

    static void push(lua_State*, const LuaRef& r)
    {
        r.pushToStack();
    }

    static LuaRef get(lua_State* L, int index)
    {
        return lua_isnone(L, index) ? LuaRef() : LuaRef(L, index);
    }

    static LuaRef opt(lua_State* L, int index, const LuaRef& def)
    {
        return lua_isnone(L, index) ? def : LuaRef(L, index);
    }
};

template<> struct LuaType <LuaRef> : LuaRefType {};
template<> struct LuaType <LuaRef&> : LuaRefType {};
template<> struct LuaType <LuaRef const&> : LuaRefType {};

//---------------------------------------------------------------------------

#if LUAINTF_HEADERS_ONLY
#include "src/LuaRef.cpp"
#endif

//---------------------------------------------------------------------------

}

#endif
