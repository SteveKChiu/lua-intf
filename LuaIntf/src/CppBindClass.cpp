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
    #include "LuaIntf.h"
    using namespace LuaIntf;
#endif

//---------------------------------------------------------------------------

LUA_INLINE int CppBindClassMetaMethod::index(lua_State* L)
{
    // <SP:1> -> table or userdata
    // <SP:2> -> key

    // get signature metatable -> <mt> <sign_mt>
    lua_getmetatable(L, 1);
    lua_rawgetp(L, -1, CppBindClassMetaMethod::signature());
    lua_rawget(L, LUA_REGISTRYINDEX);

    // check if both are equal
    if (!lua_rawequal(L, -1, -2)) {
        // not match, panic now -> <error>
        return luaL_error(L, "access '%s' : metatable is invalid", lua_tostring(L, 2));
    } else {
        // matched, pop <sign_mt> -> <mt>
        lua_pop(L, 1);
    }

    for (;;) {
        // push metatable[key] -> <mt> <mt[key]>
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);

        if (!lua_isnil(L, -1)) {
            // key value is found, pop metatable and leave key value on top -> <mt[key]>
            lua_remove(L, -2);
            break;
        } else {
            // not found, pop nil result -> <mt>
            lua_pop(L, 1);
        }

        // get metatable.getters -> <mt> <getters>
        lua_pushliteral(L, "___getters");
        lua_rawget(L, -2);
        assert(lua_istable(L, -1));

        // get metatable.getters[key] -> <mt> <getters[key]>
        lua_pushvalue(L, 2);            // push key
        lua_rawget(L, -2);              // lookup key in getters
        lua_remove(L, -2);              // pop getters

        if (lua_iscfunction(L, -1)) {
            // function found, call the getter function -> <result>
            lua_remove(L, -2);          // pop <mt>

            // now need to test whether this is object (== userdata)
            int n = 0;
            if (lua_isuserdata(L, 1)) {
                lua_pushvalue(L, 1);    // push userdata as object param for class method
                n++;
            } else {
                assert(lua_istable(L, 1));
            }

            lua_call(L, n, 1);
            break;
        } else {
            // pop nil result -> <mt>
            assert(lua_isnil(L, -1));
            lua_pop(L, 1);

            // now try super metatable -> <super_mt>
            lua_pushliteral(L, "___super");
            lua_rawget(L, -2);
            lua_remove(L, -2);

            // check if there is one
            if (lua_isnil(L, -1)) {
                // give up, leave nil on top -> <nil>
                break;
            } else {
                // yes, now continue with <super_mt>
                assert(lua_istable(L, -1));
            }
        }
    }

    return 1;
}

LUA_INLINE int CppBindClassMetaMethod::newIndex(lua_State* L)
{
    // <SP:1> -> table or userdata
    // <SP:2> -> key
    // <SP:3> -> value

    // get signature metatable -> <mt> <sign_mt>
    lua_getmetatable(L, 1);
    lua_rawgetp(L, -1, CppBindClassMetaMethod::signature());
    lua_rawget(L, LUA_REGISTRYINDEX);

    // check if both are equal
    if (!lua_rawequal(L, -1, -2)) {
        // not match, panic now -> <error>
        return luaL_error(L, "access '%s' : metatable is invalid", lua_tostring(L, 2));
    } else {
        // matched, pop <sign_mt> -> <mt>
        lua_pop(L, 1);
    }

    for (;;) {
        // get setters subtable of metatable -> <mt> <setters>
        lua_pushliteral(L, "___setters");
        lua_rawget(L, -2);              // get __setters table
        assert(lua_istable(L, -1));

        // get setters[key] -> <mt> <setters[key]>
        lua_pushvalue(L, 2);            // push key arg2
        lua_rawget(L, -2);              // lookup key in setters
        lua_remove(L, -2);              // pop setters

        if (lua_iscfunction(L, -1)) {
            // if value is a cfunction, call function(value)
            lua_remove(L, -2);          // pop metatable

            // now need to test whether this is object (== userdata)
            int n = 1;
            if (lua_isuserdata(L, 1)) {
                lua_pushvalue(L, 1);    // push userdata as object param for class method
                n++;
            } else {
                assert(lua_istable(L, 1));
            }

            lua_pushvalue(L, 3);        // push new value as arg
            lua_call(L, n, 0);          // call cfunction
            break;
        } else {
            // pop nil result -> <mt>
            assert(lua_isnil(L, -1));
            lua_pop(L, 1);

            // now try super metatable -> <super_mt>
            lua_pushliteral(L, "___super");
            lua_rawget(L, -2);
            lua_remove(L, -2);

            // check if there is one
            if (lua_isnil(L, -1)) {
                // give up, pop it
                return luaL_error(L, "no writable class member '%s'", lua_tostring(L, 2));
            } else {
                // yes, now continue with <super_mt>
                assert(lua_istable(L, -1));
            }
        }
    }

    return 0;
}

LUA_INLINE int CppBindClassMetaMethod::errorReadOnly(lua_State* L)
{
    return luaL_error(L, "class member '%s' is read-only",
        lua_tostring(L, lua_upvalueindex(1)));
}

LUA_INLINE int CppBindClassMetaMethod::errorConstMismatch(lua_State* L)
{
    return luaL_error(L, "class member function '%s' can not be access by const object",
        lua_tostring(L, lua_upvalueindex(1)));
}

//---------------------------------------------------------------------------

LUA_INLINE bool CppBindClassBase::buildMetaTable(LuaRef& meta, LuaRef& parent, const char* name,
    void* static_id, void* clazz_id, void* const_id)
{
    LuaRef ref = parent.rawget<LuaRef>(name);
    if (ref != nullptr) {
        meta = ref;
        return false;
    }

    auto L = parent.state();
    std::string type_name = "class<" + CppBindModule::getFullName(parent, name) + ">";

    LuaRef type_const = LuaRef::fromPointer(L, const_id);
    LuaRef type_clazz = LuaRef::fromPointer(L, clazz_id);
    LuaRef type_static = LuaRef::fromPointer(L, static_id);

    LuaRef clazz_const = LuaRef::createTable(L);
    clazz_const.setMetaTable(clazz_const);
    clazz_const.rawset("__index", &CppBindClassMetaMethod::index);
    clazz_const.rawset("__newindex", &CppBindClassMetaMethod::newIndex);
    clazz_const.rawset("___getters", LuaRef::createTable(L));
    clazz_const.rawset("___setters", LuaRef::createTable(L));
    clazz_const.rawset("___type", "const_" + type_name);
    clazz_const.rawset("___const", clazz_const);
    clazz_const.rawset(CppBindClassMetaMethod::signature(), type_const);

    LuaRef clazz = LuaRef::createTable(L);
    clazz.setMetaTable(clazz);
    clazz.rawset("__index", &CppBindClassMetaMethod::index);
    clazz.rawset("__newindex", &CppBindClassMetaMethod::newIndex);
    clazz.rawset("___getters", clazz_const.rawget<LuaRef>("___getters"));
    clazz.rawset("___setters", LuaRef::createTable(L));
    clazz.rawset("___type", type_name);
    clazz.rawset("___const", clazz_const);
    clazz.rawset(CppBindClassMetaMethod::signature(), type_clazz);

    LuaRef clazz_static = LuaRef::createTable(L);
    clazz_static.setMetaTable(clazz_static);
    clazz_static.rawset("__index", &CppBindClassMetaMethod::index);
    clazz_static.rawset("__newindex", &CppBindClassMetaMethod::newIndex);
    clazz_static.rawset("___getters", LuaRef::createTable(L));
    clazz_static.rawset("___setters", LuaRef::createTable(L));
    clazz_static.rawset("___type", "static_" + type_name);
    clazz_static.rawset("___class", clazz);
    clazz_static.rawset("___const", clazz_const);
    clazz_static.rawset("___parent", parent);
    clazz_static.rawset(CppBindClassMetaMethod::signature(), type_static);

    LuaRef registry(L, LUA_REGISTRYINDEX);
    registry.rawset(type_clazz, clazz);
    registry.rawset(type_const, clazz_const);
    registry.rawset(type_static, clazz_static);
    parent.rawset(name, clazz_static);

    meta = clazz_static;
    return true;
}

LUA_INLINE bool CppBindClassBase::buildMetaTable(LuaRef& meta, LuaRef& parent, const char* name,
    void* static_id, void* clazz_id, void* const_id, void* super_static_id)
{
    if (buildMetaTable(meta, parent, name, static_id, clazz_id, const_id)) {
        LuaRef registry(parent.state(), LUA_REGISTRYINDEX);
        LuaRef super = registry.rawget<LuaRef>(super_static_id);
        meta.rawset("___super", super);
        meta.rawget<LuaRef>("___class").rawset("___super", super.rawget<LuaRef>("___class"));
        meta.rawget<LuaRef>("___const").rawset("___super", super.rawget<LuaRef>("___const"));
        return true;
    }
    return false;
}

LUA_INLINE void CppBindClassBase::setStaticGetter(const char* name, const LuaRef& getter)
{
    m_meta.rawget<LuaRef>("___getters").rawset(name, getter);
}

LUA_INLINE void CppBindClassBase::setStaticSetter(const char* name, const LuaRef& setter)
{
    m_meta.rawget<LuaRef>("___setters").rawset(name, setter);
}

LUA_INLINE void CppBindClassBase::setStaticReadOnly(const char* name)
{
    setStaticSetter(name, LuaRef::createFunctionWithArgs(L(), &CppBindClassMetaMethod::errorReadOnly, name));
}

LUA_INLINE void CppBindClassBase::setMemberGetter(const char* name, const LuaRef& getter)
{
    m_meta.rawget<LuaRef>("___class").rawget<LuaRef>("___getters").rawset(name, getter);
}

LUA_INLINE void CppBindClassBase::setMemberSetter(const char* name, const LuaRef& setter)
{
    LuaRef err = LuaRef::createFunctionWithArgs(L(), &CppBindClassMetaMethod::errorConstMismatch, name);
    m_meta.rawget<LuaRef>("___class").rawget<LuaRef>("___setters").rawset(name, setter);
    m_meta.rawget<LuaRef>("___const").rawget<LuaRef>("___setters").rawset(name, err);
}

LUA_INLINE void CppBindClassBase::setMemberReadOnly(const char* name)
{
    LuaRef err = LuaRef::createFunctionWithArgs(L(), &CppBindClassMetaMethod::errorReadOnly, name);
    m_meta.rawget<LuaRef>("___class").rawget<LuaRef>("___setters").rawset(name, err);
    m_meta.rawget<LuaRef>("___const").rawget<LuaRef>("___setters").rawset(name, err);
}

LUA_INLINE void CppBindClassBase::setMemberFunction(const char* name, const LuaRef& proc, bool is_const)
{
    m_meta.rawget<LuaRef>("___class").rawset(name, proc);
    m_meta.rawget<LuaRef>("___const").rawset(name,
        is_const ? proc : LuaRef::createFunctionWithArgs(L(), &CppBindClassMetaMethod::errorConstMismatch, name));
}

LUA_INLINE CppBindModule CppBindClassBase::endClass()
{
    return CppBindModule(m_meta.rawget<LuaRef>("___parent"));
}
