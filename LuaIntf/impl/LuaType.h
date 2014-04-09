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

enum class LuaTypeID
{
    None = LUA_TNONE,
    Nil = LUA_TNIL,
    String = LUA_TSTRING,
    Number = LUA_TNUMBER,
    Thread = LUA_TTHREAD,
    Boolean = LUA_TBOOLEAN,
    Function = LUA_TFUNCTION,
    Table = LUA_TTABLE,
    Userdata = LUA_TUSERDATA,
    LightUserdata = LUA_TLIGHTUSERDATA
};

template <typename T>
struct LuaType;

//---------------------------------------------------------------------------

template <typename CPP_VALUE, typename LUA_VALUE = CPP_VALUE>
struct LuaValueType;

template <>
struct LuaValueType <bool>
{
    typedef bool ValueType;

    static void push(lua_State* L, bool value)
    {
        lua_pushboolean(L, value);
    }

    static bool get(lua_State* L, int index)
    {
        return lua_toboolean(L, index);
    }

    static bool opt(lua_State* L, int index, bool def)
    {
        return lua_isnone(L, index) ? def : lua_toboolean(L, index);
    }
};

template <typename T>
struct LuaValueType <T, lua_Integer>
{
    typedef T ValueType;

    static void push(lua_State* L, ValueType value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }

    static ValueType get(lua_State* L, int index)
    {
        return static_cast<ValueType>(luaL_checkinteger(L, index));
    }

    static ValueType opt(lua_State* L, int index, ValueType def)
    {
        return static_cast<ValueType>(luaL_optinteger(L, index, static_cast<lua_Integer>(def)));
    }
};

template <typename T>
struct LuaValueType <T, lua_Unsigned>
{
    typedef T ValueType;

    static void push(lua_State* L, ValueType value)
    {
        lua_pushunsigned(L, static_cast<lua_Unsigned>(value));
    }

    static ValueType get(lua_State* L, int index)
    {
        return static_cast<ValueType>(luaL_checkunsigned(L, index));
    }

    static ValueType opt(lua_State* L, int index, ValueType def)
    {
        return static_cast<ValueType>(luaL_optunsigned(L, index, static_cast<lua_Unsigned>(def)));
    }
};

template <typename T>
struct LuaValueType <T, lua_Number>
{
    typedef T ValueType;

    static void push(lua_State* L, ValueType value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }

    static ValueType get(lua_State* L, int index)
    {
        return static_cast<ValueType>(luaL_checknumber(L, index));
    }

    static ValueType opt(lua_State* L, int index, ValueType def)
    {
        return static_cast<ValueType>(luaL_optnumber(L, index, static_cast<lua_Number>(def)));
    }
};

template <>
struct LuaValueType <lua_CFunction>
{
    typedef lua_CFunction ValueType;

    static void push(lua_State* L, lua_CFunction f)
    {
        lua_pushcfunction(L, f);
    }

    static lua_CFunction get(lua_State* L, int index)
    {
        return lua_tocfunction(L, index);
    }

    static lua_CFunction opt(lua_State* L, int index, lua_CFunction)
    {
        return lua_tocfunction(L, index);
    }
};

template <>
struct LuaValueType <char>
{
    typedef char ValueType;

    static void push(lua_State* L, char value)
    {
        char str[] = { static_cast<char>(value), 0 };
        lua_pushstring(L, str);
    }

    static char get(lua_State* L, int index)
    {
        return luaL_checkstring(L, index)[0];
    }

    static char opt(lua_State* L, int index, char def)
    {
        char str[] = { static_cast<char>(def), 0 };
        return luaL_optstring(L, index, str)[0];
    }
};

template <>
struct LuaValueType <std::string>
{
    typedef std::string ValueType;

    static void push(lua_State* L, const std::string& str)
    {
        lua_pushlstring(L, str.data(), str.length());
    }

    static std::string get(lua_State* L, int index)
    {
        size_t len;
        const char* p = luaL_checklstring(L, index, &len);
        return std::string(p, len);
    }

    static std::string opt(lua_State* L, int index, const std::string& def)
    {
        size_t len;
        const char* p = luaL_optlstring(L, index, def.c_str(), &len);
        return std::string(p, len);
    }
};

template <typename T>
struct LuaValueType <T, const char*>
{
    typedef const char* ValueType;

    static void push(lua_State* L, const char* str)
    {
        if (str != nullptr) {
            lua_pushstring(L, str);
        } else {
            lua_pushnil(L);
        }
    }

    static const char* get(lua_State* L, int index)
    {
        return luaL_checkstring(L, index);
    }

    static const char* opt(lua_State* L, int index, const char* def)
    {
        return luaL_optstring(L, index, def);
    }
};

template <size_t N>
    struct LuaType <char[N]> : LuaValueType <const char*> {};

template <size_t N>
    struct LuaType <char(&)[N]> : LuaValueType <const char*> {};

template <size_t N>
    struct LuaType <const char[N]> : LuaValueType <const char*> {};

template <size_t N>
    struct LuaType <const char(&)[N]> : LuaValueType <const char*> {};

//---------------------------------------------------------------------------

struct LuaString
{
    LuaString()
        : data(nullptr)
        , size(0)
        {}

    LuaString(const std::string& str)
        : data(str.data())
        , size(str.size())
        {}

    LuaString(const char* str)
        : data(str)
        , size(std::strlen(str))
        {}

    LuaString(const char* str, size_t len)
        : data(str)
        , size(len)
        {}

    LuaString(lua_State* L, int index)
    {
        data = luaL_checklstring(L, index, &size);
    }

    explicit operator bool () const
    {
        return data != nullptr;
    }

    const char* data;
    size_t size;
};

template <>
struct LuaValueType <LuaString>
{
    typedef LuaString ValueType;

    static void push(lua_State* L, const LuaString& str)
    {
        if (str.data != nullptr) {
            lua_pushlstring(L, str.data, str.size);
        } else {
            lua_pushnil(L);
        }
    }

    static LuaString get(lua_State* L, int index)
    {
        return LuaString(L, index);
    }

    static LuaString opt(lua_State* L, int index, const LuaString& def)
    {
        if (lua_isnoneornil(L, index)) return def;
        return LuaString(L, index);
    }
};

//---------------------------------------------------------------------------

#if LUAINTF_UNSAFE_INT64

template <typename T>
struct LuaUnsafeInt64Type
{
    typedef T ValueType;

    static void push(lua_State* L, ValueType value)
    {
        lua_Number f = static_cast<lua_Number>(value);
#if LUAINTF_UNSAFE_INT64_CHECK
        ValueType verify = static_cast<ValueType>(f);
        if (value != verify) {
            luaL_error(L, "unsafe cast from 64-bit int");
        }
#endif
        lua_pushnumber(L, f);
    }

    static ValueType get(lua_State* L, int index)
    {
        return static_cast<ValueType>(luaL_checknumber(L, index));
    }

    static ValueType opt(lua_State* L, int index, ValueType def)
    {
        if (lua_isnoneornil(L, index)) return def;
        return static_cast<ValueType>(luaL_checknumber(L, index));
    }
};

template <>
struct LuaValueType <long long, lua_Number> : LuaUnsafeInt64Type <long long> {};

template <>
struct LuaValueType <unsigned long long, lua_Number> : LuaUnsafeInt64Type <unsigned long long> {};

#endif

//---------------------------------------------------------------------------

#define LUA_USING_VALUE_TYPE_EXT(T, V) \
    template <> struct LuaType <T> : LuaValueType <T, V> {}; \
    template <> struct LuaType <T&> : LuaValueType <T, V> {}; \
    template <> struct LuaType <T const&> : LuaValueType <T, V> {};

#define LUA_USING_VALUE_TYPE(T) \
    LUA_USING_VALUE_TYPE_EXT(T, T)

LUA_USING_VALUE_TYPE(bool)
LUA_USING_VALUE_TYPE(char)
LUA_USING_VALUE_TYPE_EXT(unsigned char, lua_Integer)
LUA_USING_VALUE_TYPE_EXT(short, lua_Integer)
LUA_USING_VALUE_TYPE_EXT(unsigned short, lua_Integer)
LUA_USING_VALUE_TYPE_EXT(int, lua_Integer)
LUA_USING_VALUE_TYPE_EXT(unsigned int, lua_Unsigned)
LUA_USING_VALUE_TYPE_EXT(long, lua_Integer)
LUA_USING_VALUE_TYPE_EXT(unsigned long, lua_Unsigned)
LUA_USING_VALUE_TYPE_EXT(float, lua_Number)
LUA_USING_VALUE_TYPE_EXT(double, lua_Number)
LUA_USING_VALUE_TYPE(lua_CFunction)
LUA_USING_VALUE_TYPE(std::string)
LUA_USING_VALUE_TYPE(const char*)
LUA_USING_VALUE_TYPE_EXT(char*, const char*)
LUA_USING_VALUE_TYPE(LuaString)

#if LUAINTF_UNSAFE_INT64
    LUA_USING_VALUE_TYPE_EXT(long long, lua_Number)
    LUA_USING_VALUE_TYPE_EXT(unsigned long long, lua_Number)
#endif
