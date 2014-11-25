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

/**
 * A template for temporately object created as lua function (with proper gc)
 * This is usually used for iterator function.
 *
 * To use this, user need to inherit CppFunctor, and override run method and optional descructor.
 * Then call pushToStack to create the functor object on lua stack.
 */
class CppFunctor
{
public:
    /**
     * Override destructor if you need to perform any cleanup action
     */
    virtual ~CppFunctor() {}

    /**
     * Override this method to perform lua function
     */
    virtual int run(lua_State* L) = 0;

    /**
     * Create the specified functor as lua function on stack
     */
    static void pushToStack(lua_State* L, CppFunctor* f);

private:
    static int call(lua_State* L);
    static int gc(lua_State* L);
};

//----------------------------------------------------------------------------

template <typename FN>
struct LuaCppFunctionWrapper
{
    [[noreturn]]
    static const FN& get(lua_State* L, int)
    {
        luaL_error(L, "invalid c++ function reference");
    }
};

#if LUAINTF_LUA_FUNCTION_WRAPPER

template <typename R, typename... P>
struct LuaCppFunctionWrapper <std::function<R(P...)>>
{
    static std::function<R(P...)> get(lua_State* L, int index)
    {
        LuaRef ref(L, index);
        return [ref] (P&&... arg) mutable {
            return ref.call<R>(std::forward<P>(arg)...);
        };
    }
};

#endif

//----------------------------------------------------------------------------

/**
 * Lua conversion for std::function type
 */
template <typename FN>
struct LuaCppFunction
{
    using ValueType = FN;
    using ReturnType = typename std::conditional<LUAINTF_LUA_FUNCTION_WRAPPER, FN, const FN&>::type;

    static void push(lua_State* L, const ValueType& proc)
    {
        using CppProc = CppBindMethod<ValueType>;
        LuaRef ref = LuaRef::createUserDataFrom(L, proc);
        ref.pushToStack();
        lua_pushlightuserdata(L, CppSignature<ValueType>::value());
        lua_pushcclosure(L, &CppProc::call, 2);
    }

    static ReturnType get(lua_State* L, int index)
    {
        index = lua_absindex(L, index);
        if (lua_iscfunction(L, index)) {
            const char* name = lua_getupvalue(L, index, 1);
            if (name && lua_isuserdata(L, -1)) {
                name = lua_getupvalue(L, index, 2);
                if (name && lua_touserdata(L, -1) == CppSignature<ValueType>::value()) {
                    const ValueType& func = *reinterpret_cast<const ValueType*>(lua_touserdata(L, -2));
                    assert(func);
                    lua_pop(L, 2);
                    return func;
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }

        return LuaCppFunctionWrapper<ValueType>::get(L, index);
    }

    static ReturnType opt(lua_State* L, int index, const ValueType& def)
    {
        if (lua_isnoneornil(L, index)) {
            return def;
        } else {
            return get(L, index);
        }
    }
};

template <typename R, typename... P>
struct LuaType <R(*)(P...)>
    : LuaCppFunction <R(*)(P...)> {};

template <typename R, typename... P>
struct LuaType <R(* &)(P...)>
    : LuaCppFunction <R(*)(P...)> {};

template <typename R, typename... P>
struct LuaType <R(* const)(P...)>
    : LuaCppFunction <R(*)(P...)> {};

template <typename R, typename... P>
struct LuaType <R(* const&)(P...)>
    : LuaCppFunction <R(*)(P...)> {};

template <typename R, typename... P>
struct LuaType <std::function<R(P...)>>
    : LuaCppFunction <std::function<R(P...)>> {};

template <typename R, typename... P>
struct LuaType <std::function<R(P...)> &>
    : LuaCppFunction <std::function<R(P...)>> {};

template <typename R, typename... P>
struct LuaType <std::function<R(P...)> const>
    : LuaCppFunction <std::function<R(P...)>> {};

template <typename R, typename... P>
struct LuaType <std::function<R(P...)> const&>
    : LuaCppFunction <std::function<R(P...)>> {};
