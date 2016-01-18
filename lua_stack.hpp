#ifndef LUA_MANIP_HPP
#define LUA_MANIP_HPP
#include <lua5.2/lua.hpp>
#include <lua5.2/lauxlib.h>
#include <tuple>

namespace luaglue
{

template <typename Type_, typename ...Args_>
struct Aggregate
{
    typedef Type_ Type;
    enum { NUM_ARGS = std::tuple_size<std::tuple<Args_...> >::value };
};

namespace impl
{

template <typename T_, typename ...Args_>
struct StackManager
{
    template <typename Any_>
    struct False { enum { RESULT = false }; };

    static_assert(False<T_>::RESULT, "No Lua StackManager defined for one or more of your types. See the README for more info.");

    // Divert the error for Pop to link-time, so the static assert above will be the only one reported.
    static void Push(lua_State* L, T_ val);
    static T_ Pop(lua_State* L, int* firstParamIndex);
};

template <>
struct StackManager<int>
{
    static int Push(lua_State* L, int val)
    {
        lua_pushinteger(L, val);
        return 1;
    }

    static int Pop(lua_State* L, int* firstParamIndex)
    {
        //TDOD: Error Handling
        int result = lua_tointeger(L, (*firstParamIndex)--);
        return result;
    }
};

} // end impl

} // end luaglue

#endif // LUA_MANIP_HPP
