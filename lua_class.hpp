#ifndef LUA_CLASS_HPP
#define LUA_CLASS_HPP
#include <vector>
#include <tuple>
#include <functional>
#include "lua_method.hpp"

namespace luaglue
{

template <typename Class_, typename ...Args_>
struct DefaultClassFactory
{
    static Class_* Create(Args_... args)
    {
        return new Class_(args...);
    }

    static void Destroy(Class_* obj)
    {
        delete obj;
        obj = nullptr;
    }
};

template <typename Class_, typename ClassFactory_, typename ...Args_>
class LuaClass
{
    typedef bool(*MethodRegistrar)(lua_State*, char const* className, char const* methodName);
    enum { NUM_ARGS = std::tuple_size<std::tuple<Args_...>>::value };

public:
    LuaClass(char const* className)
        : className_(className)
    {}

    void AddMethod(char const* methodName, MethodRegistrar func)
    {
        namedRegistrars_.emplace_back(std::make_tuple(methodName, func));
    }

    void Register(lua_State* L)
    {
        luaL_Reg lib[] = // main creation and destruction functions
        {
            { "aquire", LuaCreationWrapper },
            { "release", LuaDestructionWrapper },
            { NULL, NULL}
        };

        luaL_newlib(L, lib); // create the main table for this class: the one that has the "aquire" and "release" functions.
        lua_newtable(L); // create the table that will hold the methods and some meta-data of this class.
        lua_setfield(L, -2, "return_table"); // set the methods/meta-data table as a field of the main table.
        lua_setglobal(L, className_); // set the main table as a new global with the name of the class.
        lua_pop(L, 1); // pop the main table of the stack to clean up.

        // call all of the register functions passing in the lua state.
        for(auto iter = namedRegistrars_.begin(); iter != namedRegistrars_.end(); ++iter)
        {
            std::get<1>(*iter)(L, className_, std::get<0>((*iter)));
        }
    }

private:
    static int LuaCreationWrapper(lua_State* L)
    {
        using namespace impl;

        if(!lua_istable(L, 1))
        {
            luaL_error(L, "expected class table as first parameter; did you forget the ':' in ClassName:func() ?");
            return 0;
        }
        lua_getfield(L, 1, "return_table");

        int firstParam = lua_gettop(L)-1;
        lua_pushlightuserdata(L, ClassFactory_::Create(StackManager<Args_>::Pop(L, &firstParam)...));
        lua_setfield(L, -2, "instance");
        return 1;
    }

    static int LuaDestructionWrapper(lua_State* L)
    {
        if(!lua_istable(L, -1))
        {
            luaL_error(L, "expected class table as first parameter; did you forget the ':' in ClassName:func() ?");
            return 0;
        }

        lua_getfield(L, -1, "instance");
        Class_* obj = (Class_*)lua_touserdata(L, -1);
        assert(obj);
        ClassFactory_::Destroy(obj);
        return 0;
    }

private:
    std::vector<std::tuple<char const*, MethodRegistrar>> namedRegistrars_;
    char const* className_;
};

template<typename Result_, typename Class_, typename ...MethodArgs_>
LuaMethod<Result_, Class_, MethodArgs_...> MakeMethod(Result_ (Class_::*Func)(MethodArgs_...))
{
    (void)Func; // unused parameter used for type deduction.
    return LuaMethod<Result_, Class_, MethodArgs_...>();
}

} // luaglue

// temporary working AddMethod implemetation.
#define LG_MEMFUN(ptr) &(luaglue::MakeMethod(ptr).Register<ptr>)
#define LG_ADD_METHOD(obj, name, ptr) obj.AddMethod(name, LG_MEMFUN(ptr))

#endif // LUA_CLASS_HPP
