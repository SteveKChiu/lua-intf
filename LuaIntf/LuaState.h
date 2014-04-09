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

#ifndef LUASTATE_H
#define LUASTATE_H

//---------------------------------------------------------------------------

/**
 * Set LUAINTF_HEADERS_ONLY to 1 if you want this to be headers only library;
 * otherwise you need to compile some of the .cpp file separately
 */
#ifndef LUAINTF_HEADERS_ONLY
#define LUAINTF_HEADERS_ONLY 1
#endif

/**
 * Set LUAINTF_BUILD_LUA_CXX to 1 if you compile Lua library code as C++;
 * it is highly recommended to build Lua library as C++, so the exception handling
 * will work correctly
 */
#ifndef LUAINTF_BUILD_LUA_CXX
#define LUAINTF_BUILD_LUA_CXX 1
#endif

/**
 * Set LUAINTF_UNSAFE_INT64 to 1 if you want to include partial int64_t support
 */
#ifndef LUAINTF_UNSAFE_INT64
#define LUAINTF_UNSAFE_INT64 1
#endif

/**
 * Set LUAINTF_UNSAFE_INT64_CHECK to 1 if you want to check every pushed int64_t is safe,
 * that is, it can be converted from/to lua_Number without loss).
 * The check will throw Lua runtime error if the conversion is not safe.
 */
#ifndef LUAINTF_UNSAFE_INT64_CHECK
#define LUAINTF_UNSAFE_INT64_CHECK 0
#endif

//---------------------------------------------------------------------------

#include <cassert>
#include <cstring>
#include <string>
#include <exception>

#if !LUAINTF_BUILD_LUA_CXX
extern "C"
{
#endif

#include "lualib.h"
#include "lauxlib.h"

#if !LUAINTF_BUILD_LUA_CXX
}
#endif

#if LUAINTF_HEADERS_ONLY
#define LUA_INLINE inline
#else
#define LUA_INLINE
#endif

namespace LuaIntf
{

#include "impl/LuaException.h"
#include "impl/LuaType.h"

//---------------------------------------------------------------------------

namespace Lua
{
    /**
     * Push value onto Lua stack
     */
    template <typename T>
    inline void push(lua_State* L, T&& v)
    {
        LuaType<T>::push(L, std::forward<T>(v));
    }

    /**
     * Push string value onto Lua stack
     */
    inline void push(lua_State* L, const char* v, int len)
    {
        lua_pushlstring(L, v, len);
    }

    /**
     * Push nil onto Lua stack
     */
    inline void push(lua_State* L, std::nullptr_t)
    {
        lua_pushnil(L);
    }

    /**
     * Get value from Lua stack, stack is not changed
     */
    template <typename T>
    inline T get(lua_State* L, int index)
    {
        return LuaType<T>::get(L, index);
    }

    /**
     * Get value from Lua stack, stack is not changed,
     * you can specify optional value if the value is nil or none
     */
    template <typename T>
    inline T opt(lua_State* L, int index, const T& def)
    {
        return LuaType<T>::opt(L, index, def);
    }

    /**
     * Pop the value from top of Lua stack,
     * the top value of Lua stack is removed
     */
    template <typename T>
    inline T pop(lua_State* L)
    {
        T v = LuaType<T>::get(L, -1);
        lua_pop(L, 1);
        return v;
    }

    /**
     * Pop the value from the given index of Lua stack,
     * the value at the given index is removed
     */
    template <typename T>
    inline T pop(lua_State* L, int index)
    {
        T v = LuaType<T>::get(L, index);
        lua_remove(L, index);
        return v;
    }

    /**
     * Push STL-style list as Lua table onto Lua stack.
     */
    template <typename LIST>
    inline void pushList(lua_State* L, const LIST& list)
    {
        lua_newtable(L);
        int i = 1;
        for (auto& v : list) {
            push(L, v);
            lua_rawseti(L, -2, i++);
        }
    }

    /**
     * Get STL-style list from Lua table at the given index.
     */
    template <typename LIST>
    inline LIST getList(lua_State* L, int index)
    {
        luaL_checktype(L, index, LUA_TTABLE);
        LIST list;
        int n = luaL_len(L, index);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, index, i);
            list += pop<typename LIST::value_type>(L);
        }
        return list;
    }

    /**
     * Push STL-style map as Lua table onto Lua stack.
     */
    template <typename MAP>
    inline void pushMap(lua_State* L, const MAP& map)
    {
        lua_newtable(L);
        for (auto it = map.begin(); it != map.end(); ++it) {
            push(L, it.key());
            push(L, it.value());
            lua_settable(L, -3);
        }
    }

    /**
     * Get STL-style map from Lua table at the given index.
     */
    template <typename MAP>
    inline MAP getMap(lua_State* L, int index)
    {
        luaL_checktype(L, index, LUA_TTABLE);
        MAP map;
        lua_pushnil(L);
        while (lua_next(L, index)) {
            typename MAP::key_type key = get<typename MAP::key_type>(L, -2);
            map[key] = pop<typename MAP::mapped_type>(L);
        }
        return map;
    }

    /**
     * Push the named global onto Lua stack, the name may contains '.' to access field of table.
     * If any sub-table does not exist, this function will return nil.
     * If any value in the name path is not accessable (not a table or no __index meta-method),
     * it may result in Lua error.
     */
    void pushGlobal(lua_State* L, const char* name);

    /**
     * Pop value from top of Lua stack, and set it to the named global,
     * the name may contains '.' to access field of table.
     * If any sub-table does not exist or is not accessable (not a table or no __index meta-method),
     * it may result in Lua error.
     */
    void popToGlobal(lua_State* L, const char* name);

    /**
     * Get the named global, the Lua stack is not changed
     */
    template <typename T>
    inline T getGlobal(lua_State* L, const char* name)
    {
        pushGlobal(L, name);
        return pop<T>(L);
    }

    /**
     * Set the named global to the specified value, the Lua stack is not changed
     */
    template <typename T>
    inline void setGlobal(lua_State* L, const char* name, T&& v)
    {
        LuaType<T>::push(L, std::forward<T>(v));
        popToGlobal(L, name);
    }

    /**
     * Execute the given Lua expression,
     * if you need to return result from the expression, you have to use Lua 'return' keywoard
     * and specify the number of results to be returned. For example:
     *
     * Lua::exec(L, "return x + y", 1);
     * int r = Lua::pop<int>(L);
     *
     * The same code can be rewritten by using eval:
     *
     * int r = Lua::eval<int>("x + y");
     *
     * @param L the lua state
     * @param lua_expr the lua expression
     * @param num_results the number of values to be returned from expression
     */
    void exec(lua_State* L, const char* lua_expr, int num_results = 0);

    /**
     * Envaluate the given Lua expression, and return the value
     *
     * @param L the lua state
     * @param lua_expr the lua expression
     */
    template <typename T>
    inline T eval(lua_State* L, const char* lua_expr)
    {
        std::string expr("return ");
        expr += lua_expr;
        exec(L, expr.c_str(), 1);
        return pop<T>(L);
    }
}

//---------------------------------------------------------------------------

class LuaState
{
public:
    LuaState()
        : L(nullptr) {}

    LuaState(lua_State* that)
        : L(that) {}

    LuaState(const LuaState& that)
        : L(that.L) {}

    LuaState& operator = (lua_State* that)
        { L = that; return *this; }

    LuaState& operator = (const LuaState& that)
        { L = that.L; return *this; }

    operator lua_State* () const
        { return L; }

    bool operator == (lua_State* that) const
        { return L == that; }

    bool operator == (const LuaState& that) const
        { return L == that.L; }

    bool operator != (lua_State* that) const
        { return L != that; }

    bool operator != (const LuaState& that) const
        { return L != that.L; }

    explicit operator bool () const
        { return L != nullptr; }

// state manipulation

    static LuaState newState()
        { return luaL_newstate(); }

    static LuaState newState(lua_Alloc func, void* userdata = nullptr)
        { return lua_newstate(func, userdata); }

    void close()
        { if (L) { lua_close(L); L = nullptr; } }

    LuaState newThread() const
        { return lua_newthread(L); }

    lua_CFunction setPanicFunc(lua_CFunction panic_func) const
        { return lua_atpanic(L, panic_func); }

    lua_Alloc getAllocFunc(void** userdata = nullptr) const
        { return lua_getallocf(L, userdata); }

    void setAllocFunc(lua_Alloc func, void* userdata = nullptr) const
        { lua_setallocf(L, func, userdata);}

    const lua_Number* version() const
        { return lua_version(L); }

    void checkVersion() const
        { luaL_checkversion(L); }

// basic stack manipulation

    int getTop() const
        { return lua_gettop(L); }

    void setTop(int idx) const
        { lua_settop(L, idx); }

    void pushValueAt(int idx) const
        { lua_pushvalue(L, idx); }

    void remove(int idx) const
        { lua_remove(L, idx); }

    void insert(int idx)const
        { lua_insert(L, idx); }

    void replace(int idx) const
        { lua_replace(L, idx); }

    void copy(int from_idx, int to_idx) const
        { lua_copy(L, from_idx, to_idx); }

    int checkStack(int extra) const
        { return lua_checkstack(L, extra); }

    void checkStack(int extra, const char* msg) const
        { luaL_checkstack(L, extra, msg); }

    void xmove(const LuaState& from, int n) const
        { lua_xmove(from.L, L, n); }

    void pop(int n = 1) const
        { lua_pop(L, n); }

// access functions (stack -> CXX)

    bool isNumber(int idx) const
        { return lua_isnumber(L, idx); }

    bool isString(int idx) const
        { return lua_isstring(L, idx); }

    bool isCFunction(int idx) const
        { return lua_iscfunction(L, idx); }

    bool isUserData(int idx) const
        { return lua_isuserdata(L, idx); }

    bool isFunction(int idx) const
        { return lua_isfunction(L, idx); }

    bool isTable(int idx) const
        { return lua_istable(L, idx); }

    bool isLightUserData(int idx) const
        { return lua_islightuserdata(L, idx); }

    bool isNil(int idx) const
        { return lua_isnil(L, idx); }

    bool isBool(int idx) const
        { return lua_isboolean(L, idx); }

    bool isThread(int idx) const
        { return lua_isthread(L, idx); }

    bool isNone(int idx) const
        { return lua_isnone(L, idx); }

    bool isNoneOrNil(int idx) const
        { return lua_isnoneornil(L, idx); }

    int type(int idx) const
        { return lua_type(L, idx); }

    void checkType(int idx, int type) const
        { luaL_checktype(L, idx, type); }

    void checkAny(int idx) const
        { luaL_checkany(L, idx); }

    const char* typeName(int type) const
        { return lua_typename(L, type); }

    const char* typeNameAt(int idx) const
        { return luaL_typename(L, idx); }

    lua_Number toNumber(int idx, int* is_num = nullptr) const
        { return lua_tonumberx(L, idx, is_num); }

    lua_Number checkNumber(int idx) const
        { return luaL_checknumber(L, idx); }

    lua_Number optNumber(int idx, lua_Number def) const
        { return luaL_optnumber(L, idx, def); }

    lua_Integer toInteger(int idx, int* is_num = nullptr) const
        { return lua_tointegerx(L, idx, is_num); }

    lua_Integer checkInteger(int idx) const
        { return luaL_checkinteger(L, idx); }

    lua_Integer optInteger(int idx, lua_Integer def) const
        { return luaL_optinteger(L, idx, def); }

    lua_Unsigned toUnsigned(int idx, int* is_num = nullptr) const
        { return lua_tounsignedx(L, idx, is_num); }

    lua_Unsigned checkUnsigned(int idx) const
        { return luaL_checkunsigned(L, idx); }

    lua_Unsigned optUnsigned(int idx, lua_Unsigned def) const
        { return luaL_optunsigned(L, idx, def); }

    bool toBool(int idx) const
        { return lua_toboolean(L, idx); }

    const char* toString(int idx, size_t* len = nullptr) const
        { return lua_tolstring(L, idx, len); }

    const char* getString(int idx, size_t* len = nullptr) const
        { return luaL_tolstring(L, idx, len); }

    const char* checkString(int idx, size_t* len = nullptr) const
        { return luaL_checklstring(L, idx, len); }

    const char* optString(int idx, const char* def, size_t* len = nullptr) const
        { return luaL_optlstring(L, idx, def, len); }

    size_t rawlen(int idx) const
        { return lua_rawlen(L, idx); }

    lua_CFunction toCFunction(int idx) const
        { return lua_tocfunction(L, idx); }

    void* toUserData(int idx) const
        { return lua_touserdata(L, idx); }

    void* testUserData(int idx, const char* type_name) const
        { return luaL_testudata(L, idx, type_name); }

    void* checkUserData(int idx, const char* type_name) const
        { return luaL_checkudata(L, idx, type_name); }

    LuaState toThread(int idx) const
        { return lua_tothread(L, idx); }

    const void* toPointer(int idx) const
        { return lua_topointer(L, idx); }

    int checkOption(int idx, const char* def, const char* const list[]) const
        { return luaL_checkoption(L, idx, def, list); }

// comparison and arithmetic functions

    void arith(int op) const
        { lua_arith(L, op); }

    bool rawequal(int idx1, int idx2) const
        { return lua_rawequal(L, idx1, idx2); }

    int compare(int idx1, int idx2, int op) const
        { return lua_compare(L, idx1, idx2, op); }

// push functions (CXX -> stack)

    void push(std::nullptr_t) const
        { lua_pushnil(L); }

    void push(lua_Number n) const
        { lua_pushnumber(L, n); }

    void push(lua_Integer n) const
        { lua_pushinteger(L, n); }

    void push(lua_Unsigned n) const
        { lua_pushunsigned(L, n); }

    void push(const char* s, size_t len) const
        { lua_pushlstring(L, s, len); }

    void push(const char* s) const
        { lua_pushstring(L, s); }

    const char* pushvf(const char* fmt, va_list argp) const
        { return lua_pushvfstring(L, fmt, argp); }

    const char* pushf(const char* fmt, ...) const;

    void push(bool b) const
        { lua_pushboolean(L, b); }

    void pushCFunction(lua_CFunction fn, int closure = 0) const
        { lua_pushcclosure(L, fn, closure); }

    void pushLightUserData(void* p) const
        { lua_pushlightuserdata(L, p); }

    bool pushThread() const
        { return lua_pushthread(L) == 1; }

// get functions (Lua -> stack)

    void getRegistry() const
        { lua_pushvalue(L, LUA_REGISTRYINDEX); }

    void getGlobals() const
        { lua_pushglobaltable(L); }

    void getGlobal(const char* name) const
        { lua_getglobal(L, name); }

    void getTable(int table_idx) const
        { lua_gettable(L, table_idx); }

    bool getSubTable(int idx, const char* field) const
        { return luaL_getsubtable(L, idx, field); }

    void getField(int table_idx, const char* field) const
        { lua_getfield(L, table_idx, field); }

    void rawgetTable(int table_idx) const
        { lua_rawget(L, table_idx); }

    void rawgetTable(int table_idx, int i) const
        { lua_rawgeti(L, table_idx, i); }

    void rawgetTable(int table_idx, const void* p) const
        { lua_rawgetp(L, table_idx, p); }

    void newTable(int num_items = 0, int num_fields = 0) const
        { lua_createtable(L, num_items, num_fields); }

    bool newMetaTable(const char* type_name) const
        { return luaL_newmetatable(L, type_name); }

    bool getMetaTable(int table_idx) const
        { return lua_getmetatable(L, table_idx) != 0; }

    void getMetaTable(const char* type_name) const
        { luaL_getmetatable(L, type_name); }

    bool getMetaField(int table_idx, const char* field) const
        { return luaL_getmetafield(L, table_idx, field); }

    void* newUserData(size_t sz) const
        { return lua_newuserdata(L, sz); }

    void getUserValue(int idx) const
        { lua_getuservalue(L, idx); }

    bool getTableNext(int table_idx) const
        { return lua_next(L, table_idx) != 0; }

    void getTableLen(int table_idx) const
        { lua_len(L, table_idx); }

    int tableLen(int table_idx) const
        { return luaL_len(L, table_idx); }

// set functions (stack -> Lua)

    void setGlobal(const char* name) const
        { lua_setglobal(L, name); }

    void setTable(int table_idx) const
        { lua_settable(L, table_idx); }

    void setField(int table_idx, const char* k) const
        { lua_setfield(L, table_idx, k); }

    void rawsetTable(int table_idx) const
        { lua_rawset(L, table_idx); }

    void rawsetTable(int table_idx, int i) const
        { lua_rawseti(L, table_idx, i);}

    void rawsetTable(int table_idx, const void* p) const
        { lua_rawsetp(L, table_idx, p);}

    void setMetaTable(int table_idx) const
        { lua_setmetatable(L, table_idx); }

    void setMetaTable(const char* type_name) const
        { luaL_setmetatable(L, type_name); }

    void setUserValue(int idx) const
        { lua_setuservalue(L, idx); }

    void registerCFunction(const char* name, lua_CFunction fn, int closure = 0) const
        { lua_pushcclosure(L, fn, closure); lua_setglobal(L, name); }

// `load' and `call' functions (load and run Lua code)

    void call(int num_args, int num_results, int ctx = 0, lua_CFunction k = nullptr) const
        { lua_callk(L, num_args, num_results, ctx, k); }

    int pcall(int num_args, int num_results, int err_func_idx, int ctx = 0, lua_CFunction k = nullptr) const
        { return lua_pcallk(L, num_args, num_results, err_func_idx, ctx, k); }

    bool callMeta(int idx, const char* field) const
        { return luaL_callmeta(L, idx, field); }

    int load(lua_Reader reader, void* dt, const char* chunk_name, const char* mode = nullptr) const
        { return lua_load(L, reader, dt, chunk_name, mode); }

    int loadFile(const char* filename, const char* mode = nullptr) const
        { return luaL_loadfilex(L, filename, mode); }

    int loadBuffer(const char* buff, size_t sz, const char* chunk_name, const char* mode = nullptr) const
        { return luaL_loadbufferx(L, buff, sz, chunk_name, mode); }

    int loadString(const char* s) const
        { return luaL_loadstring(L, s); }

    void openLibs() const
        { luaL_openlibs(L); }

    void require(const char* mod_name, lua_CFunction open_func, bool set_global = true) const
        { luaL_requiref(L, mod_name, open_func, set_global); }

    bool doFile(const char* filename) const
        { return luaL_dofile(L, filename); }

    bool doString(const char* s) const
        { return luaL_dostring(L, s); }

    int dump(lua_Writer writer, void* data) const
        { return lua_dump(L, writer, data); }

    int fileResult(int stat, const char* file_name) const
        { return luaL_fileresult(L, stat, file_name); }

    int execResult(int stat) const
        { return luaL_execresult(L, stat); }

// coroutine functions

    int yield(int num_results, int ctx = 0, lua_CFunction k = nullptr) const
        { return lua_yieldk(L, num_results, ctx, k); }

    int resume(const LuaState& from, int num_args) const
        { return lua_resume(L, from.L, num_args); }

    int status() const
        { return lua_status(L); }

    int gc(int what, int data) const
        { return lua_gc(L, what, data); }

// miscellaneous functions

    void where(int level = 0) const
        { luaL_where(L, level); }

    int error() const
        { return lua_error(L); }

    int error(const char* fmt, ...) const;

    int argError(int idx, const char* msg) const
        { return luaL_argerror(L, idx, msg); }

    void traceback(const LuaState& that , const char* msg = nullptr, int level = 0) const
        { luaL_traceback(L, that.L, msg, level); }

// aux functions

    void concat(int n) const
        { lua_concat(L, n); }

    const char* gsub(const char* src, const char* pattern, const char* replace) const
        { return luaL_gsub(L, src, pattern, replace); }

    void getRef(int ref) const
        { lua_rawgeti(L, LUA_REGISTRYINDEX, ref); }

    int ref() const
        { return luaL_ref(L, LUA_REGISTRYINDEX); }

    void unref(int ref) const
        { luaL_unref(L, LUA_REGISTRYINDEX, ref); }

    int ref(int table_idx) const
        { return luaL_ref(L, table_idx); }

    void unref(int table_idx, int ref) const
        { luaL_unref(L, table_idx, ref); }

// debug functions

    bool getStack(int level, lua_Debug* ar) const
        { return lua_getstack(L, level, ar); }

    bool getInfo(const char* what, lua_Debug* ar) const
        { return lua_getinfo(L, what, ar) != 0; }

    const char* getLocal(const lua_Debug* ar, int n) const
        { return lua_getlocal(L, ar, n); }

    const char* setLocal(const lua_Debug* ar, int n) const
        { return lua_setlocal(L, ar, n); }

    const char* getUpvalue(int func_idx, int n) const
        { return lua_getupvalue(L, func_idx, n); }

    const char* setUpvalue(int func_idx, int n) const
        { return lua_setupvalue(L, func_idx, n); }

    void* upvalueId(int func_idx, int n) const
        { return lua_upvalueid(L, func_idx, n); }

    void upvalueJoin(int func_idx1, int n1, int func_idx2, int n2) const
        { lua_upvaluejoin(L, func_idx1, n1, func_idx2, n2);}

    int setHook(lua_Hook func, int mask, int count) const
        { return lua_sethook(L, func, mask, count); }

    lua_Hook getHook() const
        { return lua_gethook(L); }

    int getHookMask() const
        { return lua_gethookmask(L); }

    int getHookCount() const
        { return lua_gethookcount(L); }

// CXX binding

    template <typename T>
    void push(T&& v) const
        { Lua::push(L, std::forward<T>(v)); }

    template <typename T>
    T toValue(int index) const
        { return Lua::get<T>(L, index); }

    template <typename T>
    T optValue(int index, const T& def) const
        { return Lua::opt<T>(L, index, def); }

    template <typename T>
    T popValue() const
        { return Lua::pop<T>(L); }

    template <typename T>
    T toGlobalValue(const char* name) const
        { return Lua::getGlobal<T>(L, name); }

    template <typename T>
    void setGlobalValue(const char* name, T&& v) const
        { Lua::setGlobal(L, name, std::forward(v)); }

    void exec(const char* expr, int num_results = 0)
        { Lua::exec(L, expr, num_results); }

    template <typename T>
    T eval(const char* expr) const
        { return Lua::eval<T>(L, expr); }

private:
    lua_State* L;
};

//---------------------------------------------------------------------------

#if LUAINTF_HEADERS_ONLY
#include "src/LuaState.cpp"
#endif

//---------------------------------------------------------------------------

}

#endif
