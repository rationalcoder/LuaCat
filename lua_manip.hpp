#ifndef LUA_MANIP_HPP
#define LUA_MANIP_HPP
#include <lua.hpp>
#include <lauxlib.h>

namespace luaglue
{

template <typename T>
inline T LuaExtract(lua_State* L, int& firstParamIndex);

template <>
inline int LuaExtract<int>(lua_State* L, int& firstParamIndex)
{
        //TDOD: Error Handling
        int result = lua_tointeger(L, firstParamIndex--);
        return result;
}

template <typename T>
inline T LuaReturn(lua_State* L, const T&& t);

template <>
inline int LuaReturn(lua_State* L, const int&& i)
{
        lua_pushinteger(L, i);
        return 1;
}

}
#endif // LUA_MANIP_HPP
