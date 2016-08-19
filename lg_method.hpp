#ifndef LG_METHOD_HPP
#define LG_METHOD_HPP
#include <cassert>
#include "lg_common.hpp"
#include "lg_stack.hpp"

/**
 * \file lua_method.hpp
 * \brief Handles method wrapper generation.
 */


namespace luaglue
{

namespace impl
{

template <typename Result_, typename Class_, typename ...Args_>
struct LuaBaseCallWrapper
{
    typedef Result_ (Class_::*Func)(Args_...);

    static Class_* GrabInstance(lua_State* L)
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
        // object->method(args)
        return (static_cast<Class_*>(object)->*Func_)(std::forward<Args_>(args)...);
    }
};

template <typename Result_, typename Class_, typename ...Args_>
struct LuaCallWrapper
{
    using Base = LuaBaseCallWrapper<Result_, Class_, Args_...>;

    template <typename Base::Func Func_, std::size_t ...Indices_>
    static int Call(lua_State* L)
    {
        return StackManager<Result_>::Push(L, Base::template MethodWrapper<Func_>(Base::GrabInstance(L), StackManager<Args_>::At<Indices_ + 2>(L)...));
    }
};

template <typename Class_, typename ...Args_>
struct LuaCallWrapper<void, Class_, Args_...>
{
    using Base = LuaBaseCallWrapper<void, Class_, Args_...>;

    template <typename Base::Func Func_, std::size_t ...Indices_>
    static int Call(lua_State* L)
    {
        Base::template MethodWrapper<Func_>(Base::GrabInstance(L), StackManager<Args_>::template At<Indices_ + 2>(L)...);
        return 0;
    }
};

} // impl

template <typename Result_, class Class_, typename ...Args_>
class LuaMethod
{
public:
    typedef Result_ (Class_::*Func)(Args_...);

public:
    template <Func Func_, std::size_t ...Indices_>
    static bool Register(lua_State* L, char const* className, char const* methodName)
    {
        using namespace impl;
        assert(className && methodName);

        lua_getglobal(L, className);
        lua_getfield(L, -1, "return_table");
        lua_pushcfunction(L, (LuaCallWrapper<Result_, Class_, Args_...>::template Call<Func_, Indices_...>));
        lua_setfield(L, -2, methodName);

        lua_pop(L, lua_gettop(L));
        return true;
    }

    template <Func Func_>
    static constexpr impl::MethodRegistrar MakeRegistrar()
    {
        return DeduceRegistrar<Func_>(typename impl::BuildIndexSequence<sizeof ...(Args_)>::Type());
    }

private:
    template <Func Func_, std::size_t ...Indices_>
    static constexpr impl::MethodRegistrar DeduceRegistrar(impl::IndexSequence<Indices_...>)
    {
        return Register<Func_, Indices_...>;
    }
};

} // luaglue

#endif // LG_METHOD_HPP
