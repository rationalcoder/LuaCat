#ifndef LUA_METHOD_HPP
#define LUA_METHOD_HPP
#include <cassert>
#include <lua5.2/lua.hpp>
#include <lua5.2/lauxlib.h>
#include <tuple>
#include "lua_stack.hpp"

/**
 * @file lua_method.hpp
 * @brief Handles method wrapper generation.
 */


namespace luaglue
{

namespace impl
{

template <typename Result_, typename Class_, typename ...Args_>
struct LuaBaseCallWrapper
{
    typedef Result_ (Class_::*Func)(Args_...);

    static Class_* CommonWork(lua_State* L)
    {
        // TODO: Error Checking...
        lua_getfield(L, 1, "instance");
        Class_* obj = (Class_*)lua_touserdata(L, -1);
        assert(obj);
        lua_remove(L, 1);
        lua_pop(L, 1);
        return obj;
    }

    template<Func Func_>
    static Result_ MethodWrapper(void* object, Args_&&... args)
    {
        return (static_cast<Class_*>(object)->*Func_)(std::forward<Args_>(args)...);
    }
};

template <typename Result_, typename Class_, bool HasArgs_, typename ...Args_>
struct LuaCallWrapper
{
    using Base = LuaBaseCallWrapper<Result_, Class_, Args_...>;

    template <typename Base::Func Func_>
    static int Call(lua_State* L)
    {
        Class_* obj = Base::CommonWork(L);
        int i = lua_gettop(L); // looping variable that represents the stack index to extract a parameter from.
        return StackManager<Result_>::Push(L, Base::template MethodWrapper<Func_>(obj, StackManager<Args_>::Pop(L, &i)...));
    }
};

template <typename Result_, typename Class_, typename ...Args_>
struct LuaCallWrapper <Result_, Class_, false, Args_...>
{
    using Base = LuaBaseCallWrapper<Result_, Class_, Args_...>;

    template <typename Base::Func Func_>
    static int Call(lua_State* L)
    {
        Class_* obj = Base::CommonWork(L);
        return StackManager<Result_>::Push(L, Base::template MethodWrapper<Func_>(obj));
    }
};

template <typename Class_, typename ...Args_>
struct LuaCallWrapper<void, Class_, true, Args_...>
{
    using Base = LuaBaseCallWrapper<void, Class_, Args_...>;

    template <typename Base::Func Func_>
    static int Call(lua_State* L)
    {
        Class_* obj = Base::CommonWork(L);
        int i = lua_gettop(L); // looping variable that represents the stack index to extract a parameter from.
        Base::template MethodWrapper<Func_>(obj, StackManager<Args_>::Pop(L, &i)...);
        return 0;
    }
};

template <typename Class_, typename ...Args_>
struct LuaCallWrapper<void, Class_, false, Args_...>
{
    using Base = LuaBaseCallWrapper<void, Class_, Args_...>;

    template <typename Base::Func Func_>
    static int Call(lua_State* L)
    {
        (void)L;
        Class_* obj = Base::CommonWork(L);
        Base::template MethodWrapper<Func_>(obj);
        return 0;
    }
};

} // impl

template <typename Result_, class Class_, typename ...Args_>
struct LuaMethod
{
    typedef Result_ (Class_::*Func)(Args_...);
    static constexpr bool HAS_ARGS = std::tuple_size<std::tuple<Args_...>>::value;

    template <Func Func_>
    static bool Register(lua_State* L, char const* className, char const* methodName)
    {
        using namespace impl;
        assert(className && methodName);

        lua_getglobal(L, className);
        lua_getfield(L, -1, "return_table");
        lua_pushcclosure(L, LuaCallWrapper<Result_, Class_, HAS_ARGS, Args_...>::template Call<Func_>, 0);
        lua_setfield(L, -2, methodName);

        lua_pop(L, lua_gettop(L));
        return true;
    }
};

} // luaglue

#endif // LUA_METHOD_HPP
