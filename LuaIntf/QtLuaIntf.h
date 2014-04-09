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

#ifndef QTLUAINTF_H
#define QTLUAINTF_H

//---------------------------------------------------------------------------

#include "LuaIntf.h"
#include <QByteArray>
#include <QVariant>

namespace LuaIntf
{

//---------------------------------------------------------------------------

namespace Lua
{
    void pushVariant(lua_State* L, const QVariant& v);
    QVariant getVariant(lua_State* L, int index);

    template <typename LIST>
    inline void pushList(lua_State* L, const LIST& list)
    {
        lua_newtable(L);
        int i = 1;
        for (auto& v : list) {
            Lua::push(L, v);
            lua_rawseti(L, -3, i++);
        }
        lua_pop(L, 1);
    }

    template <typename LIST>
    inline LIST getList(lua_State* L, int index)
    {
        luaL_checktype(L, index, LUA_TTABLE);
        LIST list;
        int n = luaL_len(L, index);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, index, i);
            list += Lua::pop<typename LIST::value_type>(L);
        }
        return list;
    }

    template <typename LIST>
    inline LIST getList(LuaRef table)
    {
        table.pushToStack();
        LIST list = getList<LIST>(table.state(), -1);
        lua_pop(table.state(), 1);
        return list;
    }

    template <typename MAP>
    inline void pushMap(lua_State* L, const MAP& map)
    {
        lua_newtable(L);
        for (auto it = map.begin(); it != map.end(); ++it) {
            Lua::push(L, it.key());
            Lua::push(L, it.value());
            lua_settable(L, -3);
        }
        lua_pop(L, 1);
    }

    template <typename MAP>
    inline MAP getMap(lua_State* L, int index)
    {
        luaL_checktype(L, index, LUA_TTABLE);
        MAP map;
        lua_pushnil(L);
        while (lua_next(L, index)) {
            typename MAP::key_type key = Lua::get<typename MAP::key_type>(L, -2);
            map[key] = Lua::pop<typename MAP::mapped_type>(L);
        }
        return map;
    }

    template <typename MAP>
    inline MAP getMap(LuaRef table)
    {
        table.pushToStack();
        MAP map = getMap<MAP>(table.state(), -1);
        lua_pop(table.state(), 1);
        return map;
    }
}

//---------------------------------------------------------------------------

template <>
struct LuaValueType <QByteArray>
{
    typedef QByteArray ValueType;

    static void push(lua_State* L, const QByteArray& str)
    {
        if (str.isEmpty()) {
            lua_pushliteral(L, "");
        } else {
            lua_pushlstring(L, str.data(), str.length());
        }
    }

    static QByteArray get(lua_State* L, int index)
    {
        size_t len;
        const char* p = luaL_checklstring(L, index, &len);
        return QByteArray(p, len);
    }

    static QByteArray opt(lua_State* L, int index, const QByteArray& def)
    {
        if (lua_isnoneornil(L, index)) {
            return def;
        } else {
            size_t len;
            const char* p = luaL_checklstring(L, index, &len);
            return QByteArray(p, len);
        }
    }
};

//---------------------------------------------------------------------------

template <>
struct LuaValueType <QString>
{
    typedef QString ValueType;

    static void push(lua_State* L, const QString& str)
    {
        if (str.isEmpty()) {
            lua_pushliteral(L, "");
        } else {
            QByteArray buf = str.toUtf8();
            lua_pushlstring(L, buf.data(), buf.length());
        }
    }

    static QString get(lua_State* L, int index)
    {
        size_t len;
        const char* p = luaL_checklstring(L, index, &len);
        return QString::fromUtf8(p, len);
    }

    static QString opt(lua_State* L, int index, const QString& def)
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

//---------------------------------------------------------------------------

template <>
struct LuaValueType <QVariant>
{
    typedef QVariant ValueType;

    static void push(lua_State* L, const QVariant& v)
    {
        Lua::pushVariant(L, v);
    }

    static QVariant get(lua_State* L, int index)
    {
        return Lua::getVariant(L, index);
    }

    static QVariant opt(lua_State* L, int index, const QVariant& def)
    {
        if (lua_isnoneornil(L, index)) {
            return def;
        } else {
            return Lua::getVariant(L, index);
        }
    }
};

//---------------------------------------------------------------------------

template <>
struct LuaValueType <QVariantMap>
{
    typedef QVariantMap ValueType;

    static void push(lua_State* L, const QVariantMap& v)
    {
        if (v.isEmpty()) {
            lua_newtable(L);
        } else {
            Lua::pushMap(L, v);
        }
    }

    static QVariantMap get(lua_State* L, int index)
    {
        if (lua_isnoneornil(L, index)) {
            return QVariantMap();
        } else {
            return Lua::getMap<QVariantMap>(L, index);
        }
    }

    static QVariantMap opt(lua_State* L, int index, const QVariantMap& def)
    {
        if (lua_isnoneornil(L, index)) {
            return def;
        } else {
            return Lua::getMap<QVariantMap>(L, index);
        }
    }
};

//---------------------------------------------------------------------------

template <>
struct LuaValueType <QVariantList>
{
    typedef QVariantList ValueType;

    static void push(lua_State* L, const QVariantList& v)
    {
        if (v.isEmpty()) {
            lua_newtable(L);
        } else {
            Lua::pushList(L, v);
        }
    }

    static QVariantList get(lua_State* L, int index)
    {
        if (lua_isnoneornil(L, index)) {
            return QVariantList();
        } else {
            return Lua::getList<QVariantList>(L, index);
        }
    }

    static QVariantList opt(lua_State* L, int index, const QVariantList& def)
    {
        if (lua_isnoneornil(L, index)) {
            return def;
        } else {
            return Lua::getList<QVariantList>(L, index);
        }
    }
};

//---------------------------------------------------------------------------

template <>
struct LuaValueType <QStringList>
{
    typedef QStringList ValueType;

    static void push(lua_State* L, const QStringList& v)
    {
        if (v.isEmpty()) {
            lua_newtable(L);
        } else {
            Lua::pushList(L, v);
        }
    }

    static QStringList get(lua_State* L, int index)
    {
        if (lua_isnoneornil(L, index)) {
            return QStringList();
        } else {
            return Lua::getList<QStringList>(L, index);
        }
    }

    static QStringList opt(lua_State* L, int index, const QStringList& def)
    {
        if (lua_isnoneornil(L, index)) {
            return def;
        } else {
            return Lua::getList<QStringList>(L, index);
        }
    }
};

//---------------------------------------------------------------------------

LUA_USING_VALUE_TYPE(QByteArray)
LUA_USING_VALUE_TYPE(QString)
LUA_USING_VALUE_TYPE(QStringList)
LUA_USING_VALUE_TYPE(QVariant)
LUA_USING_VALUE_TYPE(QVariantList)
LUA_USING_VALUE_TYPE(QVariantMap)

#if LUAINTF_HEADERS_ONLY
#include "src/QtLuaIntf.cpp"
#endif

//---------------------------------------------------------------------------

}

#endif
