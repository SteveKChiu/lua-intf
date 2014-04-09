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

#ifndef LUAINTF_H
    #include "LuaIntf/LuaIntf.h"
    using namespace LuaIntf;
#endif

//---------------------------------------------------------------------------

LUA_INLINE int CppFunctor::call(lua_State* L)
{
    try {
        CppFunctor* f = *static_cast<CppFunctor**>(lua_touserdata(L, 1));
        return f->run(L);
    } catch (std::exception& e) {
        return luaL_error(L, e.what());
    }
}

LUA_INLINE int CppFunctor::gc(lua_State* L)
{
    try {
        CppFunctor* f = *static_cast<CppFunctor**>(lua_touserdata(L, 1));
        delete f;
        return 0;
    } catch (std::exception& e) {
        return luaL_error(L, e.what());
    }
}

LUA_INLINE void CppFunctor::pushToStack(lua_State* L, CppFunctor* f)
{
    // need to create userdata, lightuserdata can't be gc
    CppFunctor** p = static_cast<CppFunctor**>(lua_newuserdata(L, sizeof(CppFunctor*)));
    *p = f;

    lua_newtable(L);
    lua_pushcfunction(L, &call);
    lua_setfield(L, -2, "__call");
    lua_pushcfunction(L, &gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
}

//---------------------------------------------------------------------------

LUA_INLINE void CppObject::typeMismatchError(lua_State* L, int index)
{
    // <SP: index> = <obj>
    // <SP: -2> = <expected_mt>
    // <SP: -1> = <actual_mt>

    // now get the expected type -> <expected_mt> <actual_mt> <expected>
    lua_pushliteral(L, "___type");
    lua_rawget(L, -3);
    const char* expected = lua_tostring(L, -1);

    // now get the actual got type -> <expected_mt> <actual_mt> <expected> <actual>
    lua_pushliteral(L, "___type");
    lua_rawget(L, -3);
    const char* actual = lua_tostring(L, -1);
    if (actual == nullptr) {
        actual = lua_typename(L, lua_type(L, index));
    }

    // now create error msg, put it into bottom and pop all others -> <msg>
    luaL_where(L, 1);
    lua_pushfstring(L, "%s expected, got %s", expected, actual);
    lua_concat(L, 2);
    lua_error(L);
}

LUA_INLINE CppObject* CppObject::getExactObject(lua_State* L, int index, void* class_id)
{
    if (!lua_isuserdata(L, index)) {
        luaL_error(L, "expect userdata, got %s", lua_typename(L, lua_type(L, index)));
        return nullptr;
    }

    // <SP: index> = <obj>
    index = lua_absindex(L, index);

    // get object class metatable and registry class metatable -> <reg_mt> <obj_mt>
    lua_rawgetp(L, LUA_REGISTRYINDEX, class_id);
    assert(lua_istable(L, -1));
    lua_getmetatable(L, index);

    // check if <obj_mt> and <reg_mt> are equal
    if (lua_rawequal(L, -1, -2)) {
        // matched, return this object
        lua_pop(L, 2);
        return static_cast<CppObject*>(lua_touserdata(L, index));
    } else {
        // show error -> <reg_mt> <obj_mt>
        typeMismatchError(L, index);
        return nullptr;
    }
}

LUA_INLINE CppObject* CppObject::getObject(lua_State* L, int index, void* base_id, bool is_const)
{
    if (!lua_isuserdata(L, index)) {
        luaL_error(L, "expect userdata, got %s", lua_typename(L, lua_type(L, index)));
        return nullptr;
    }

    // <SP: index> = <obj>
    index = lua_absindex(L, index);

    // get registry base class metatable -> <base_mt>
    lua_rawgetp(L, LUA_REGISTRYINDEX, base_id);

    // report error if no metatable
    if (!lua_istable(L, -1)) {
        luaL_error(L, "unknown class (null metatable)");
        return nullptr;
    }

    // get the object metatable -> <base_mt> <obj_mt>
    lua_getmetatable(L, index);

    if (is_const) {
        // get the const metatable -> <base_mt> <const_obj_mt>
        lua_pushliteral(L, "___const");
        lua_rawget(L, -2);
        lua_remove(L, -2);

        // report error if no const metatable
        if (!lua_istable(L, -1)) {
            luaL_error(L, "unknown class (null const metatable)");
            return nullptr;
        }
    }

    for (;;) {
        // check if <obj_mt> and <base_mt> are equal
        if (lua_rawequal(L, -1, -2)) {
            // matched, return this object
            lua_pop(L, 2);
            break;
        }

        // now try super class -> <base_mt> <obj_mt> <obj_super_mt>
        lua_pushliteral(L, "___super");
        lua_rawget(L, -2);

        if (lua_isnil(L, -1)) {
            // pop nil
            lua_pop(L, 1);

            // show error -> <reg_mt> <obj_mt>
            typeMismatchError(L, index);
            return nullptr;
        } else {
            // continue with <obj_super_mt> -> <base_mt> <obj_super_mt>
            lua_remove(L, -2);
        }
    }

    return static_cast<CppObject*>(lua_touserdata(L, index));
}

