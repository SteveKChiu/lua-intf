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

struct _arg {};

template <typename T>
struct _opt {};

template <typename T, T DEF>
struct _def {};

template <typename T>
struct _out {};

template <typename T>
struct _ref {};

template <typename T>
struct _ref_opt {};

template <typename T, T DEF>
struct _ref_def {};

#define LUA_SP(p) static_cast<p*>(nullptr)
#define LUA_ARGS_TYPE(...) LuaIntf::_arg(*)(__VA_ARGS__)
#define LUA_ARGS(...) static_cast<LUA_ARGS_TYPE(__VA_ARGS__)>(nullptr)

#define LUA_FN(r, m, ...) static_cast<r(*)(__VA_ARGS__)>(&m)
#define LUA_MEMFN(t, r, m, ...) static_cast<r(t::*)(__VA_ARGS__)>(&t::m)

//---------------------------------------------------------------------------

template <typename T>
struct CppArgTraits
{
    typedef T Type;
    typedef typename LuaType<T>::ValueType ValueType;
    static constexpr bool IsInput = true;
    static constexpr bool IsOutput = false;
    static constexpr bool IsOptonal = false;
    static constexpr bool IsDefault = false;
};

template <typename T>
struct CppArgTraits <_opt<T>> : CppArgTraits <T>
{
    static constexpr bool IsOptonal = true;
};

template <typename T, T DEF>
struct CppArgTraits <_def<T, DEF>> : CppArgTraits <T>
{
    static constexpr bool IsOptonal = true;
    static constexpr bool IsDefault = true;
    static constexpr T DefaultValue = DEF;
};

template <typename T>
struct CppArgTraits <_out<T>> : CppArgTraits <T>
{
    static_assert(!std::is_const<T>::value && std::is_lvalue_reference<T>::value,
        "argument with out spec must be non-const reference type");
    static constexpr bool IsInput = false;
    static constexpr bool IsOutput = true;
};

template <typename T>
struct CppArgTraits <_ref<T>> : CppArgTraits <T>
{
    static_assert(!std::is_const<T>::value && std::is_lvalue_reference<T>::value,
        "argument with ref spec must be non-const reference type");
    static constexpr bool IsOutput = true;
};

template <typename T>
struct CppArgTraits <_ref_opt<T>> : CppArgTraits <T>
{
    static_assert(!std::is_const<T>::value && std::is_lvalue_reference<T>::value,
        "argument with ref spec must be non-const reference type");
    static constexpr bool IsOptonal = true;
    static constexpr bool IsOutput = true;
};

template <typename T, T DEF>
struct CppArgTraits <_ref_def<T, DEF>> : CppArgTraits <T>
{
    static_assert(!std::is_const<T>::value && std::is_lvalue_reference<T>::value,
        "argument with ref spec must be non-const reference type");
    static constexpr bool IsOptonal = true;
    static constexpr bool IsOutput = true;
    static constexpr bool IsDefault = true;
    static constexpr T DefaultValue = DEF;
};

template <>
struct CppArgTraits <lua_State*>
{
    typedef lua_State* Type;
    typedef lua_State* ValueType;
    static constexpr bool IsInput = false;
    static constexpr bool IsOutput = false;
    static constexpr bool IsOptonal = false;
    static constexpr bool IsDefault = false;
};

template <>
struct CppArgTraits <LuaState>
{
    typedef LuaState Type;
    typedef LuaState ValueType;
    static constexpr bool IsInput = false;
    static constexpr bool IsOutput = false;
    static constexpr bool IsOptonal = false;
    static constexpr bool IsDefault = false;
};

//---------------------------------------------------------------------------

template <typename Traits, bool IsInput, bool IsOptional, bool IsDefault>
struct CppArgTraitsInput;

template <typename Traits, bool IsOptional, bool IsDefault>
struct CppArgTraitsInput <Traits, false, IsOptional, IsDefault>
{
    static int get(lua_State*, int, typename Traits::ValueType&)
    {
        return 0;
    }
};

template <typename Traits, bool IsDefault>
struct CppArgTraitsInput <Traits, true, false, IsDefault>
{
    static int get(lua_State* L, int index, typename Traits::ValueType& r)
    {
        r = LuaType<typename Traits::Type>::get(L, index);
        return 1;
    }
};

template <typename Traits>
struct CppArgTraitsInput <Traits, true, true, false>
{
    static int get(lua_State* L, int index, typename Traits::ValueType& r)
    {
        r = LuaType<typename Traits::Type>::opt(L, index, typename Traits::ValueType());
        return 1;
    }
};

template <typename Traits>
struct CppArgTraitsInput <Traits, true, true, true>
{
    static int get(lua_State* L, int index, typename Traits::ValueType& r)
    {
        r = LuaType<typename Traits::Type>::opt(L, index, Traits::DefaultValue);
        return 1;
    }
};

//---------------------------------------------------------------------------

template <typename Traits, bool IsOutput>
struct CppArgTraitsOutput;

template <typename Traits>
struct CppArgTraitsOutput <Traits, false>
{
    static int push(lua_State*, const typename Traits::ValueType&)
    {
        return 0;
    }
};

template <typename Traits>
struct CppArgTraitsOutput <Traits, true>
{
    static int push(lua_State* L, const typename Traits::ValueType& f)
    {
        LuaType<typename Traits::Type>::push(L, f);
        return 1;
    }
};

//---------------------------------------------------------------------------

template <typename T>
struct CppArg
{
    typedef CppArgTraits<T> Traits;
    typedef typename Traits::Type Type;
    typedef typename Traits::ValueType ValueType;

    static int get(lua_State* L, int index, ValueType& r)
    {
        return CppArgTraitsInput<Traits, Traits::IsInput, Traits::IsOptonal, Traits::IsDefault>::get(L, index, r);
    }

    static int push(lua_State* L, const ValueType& f)
    {
        return CppArgTraitsOutput<Traits, Traits::IsOutput>::push(L, f);
    }
};

template <>
struct CppArg <lua_State*>
{
    typedef CppArgTraits<lua_State*> Traits;
    typedef typename Traits::Type Type;
    typedef typename Traits::ValueType ValueType;

    static int get(lua_State* L, int, lua_State*& r)
    {
        r = L;
        return 0;
    }

    static int push(lua_State*, lua_State*)
    {
        return 0;
    }
};

template <>
struct CppArg <LuaState>
{
    typedef CppArgTraits<LuaState> Traits;
    typedef typename Traits::Type Type;
    typedef typename Traits::ValueType ValueType;

    static int get(lua_State* L, int, LuaState& r)
    {
        r = L;
        return 0;
    }

    static int push(lua_State*, LuaState)
    {
        return 0;
    }
};

//---------------------------------------------------------------------------

template <typename... P>
struct CppArgTuple
{
    typedef std::tuple<typename CppArg<P>::ValueType...> Type;
};

//---------------------------------------------------------------------------

template <typename... P>
struct CppArgInput;

template <>
struct CppArgInput <>
{
    template <typename... T>
    static void get(lua_State*, int, std::tuple<T...>&)
    {
        // template terminate function
    }
};

template <typename P0, typename... P>
struct CppArgInput <P0, P...>
{
    template <typename... T>
    static void get(lua_State* L, int index, std::tuple<T...>& t)
    {
        index += CppArg<P0>::get(L, index, std::get<sizeof...(T) - sizeof...(P) - 1>(t));
        CppArgInput<P...>::get(L, index, t);
    }
};

//---------------------------------------------------------------------------

template <typename... P>
struct CppArgOutput;

template <>
struct CppArgOutput <>
{
    template <typename... T>
    static int push(lua_State*, const std::tuple<T...>&)
    {
        return 0;
    }
};

template <typename P0, typename... P>
struct CppArgOutput <P0, P...>
{
    template <typename... T>
    static int push(lua_State* L, const std::tuple<T...>& t)
    {
        int n = CppArg<P0>::push(L, std::get<sizeof...(T) - sizeof...(P) - 1>(t));
        return n + CppArgOutput<P...>::push(L, t);
    }
};
