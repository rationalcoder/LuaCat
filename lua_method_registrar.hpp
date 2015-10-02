#ifndef LUA_METHOD_HPP
#define LUA_METHOD_HPP
#include <cassert>
#include <lua.hpp>
#include <lauxlib.h>
#include <type_traits>
#include <tuple>
#include <functional>
#include "lua_manip.hpp"

#include <iostream>

#define REM_UNUSED_WARNING(i) ++i; --i // temporary replacement for extra template specializations to remove unused variable warnings.

namespace luaglue
{

template<typename Result_, typename Registrar_, class Class_, typename ...Args_>
struct LuaCallWrapper;

template<typename Result_, class Class_, typename ...Args_>
class LuaMethodRegistrar
{
public:
        typedef Result_ (Class_::*Func)(Args_...);
        using This_ = LuaMethodRegistrar<Result_, Class_, Args_...>;
        typedef bool(*RegisterFunc)(lua_State*, char const* className, char const* methodName);

        friend class LuaCallWrapper<Result_, This_, Class_, Args_...>;
public:
        template <Func Func_>
        static RegisterFunc GetWrapper()
        {
                return Register<Func_>;
        }

private:
        template <Func Func_>
        static bool Register(lua_State* L, char const* className, char const* methodName)
        {
                assert(className && methodName);

                lua_getglobal(L, className);
                lua_getfield(L, -1, "return_table");
                lua_pushcclosure(L, LuaCallWrapper<Result_, This_, Class_, Args_...>::template Call<Func_>, 0);
                lua_setfield(L, -2, methodName);

                lua_pop(L, lua_gettop(L));
                return true;
        }

        template<Func Func_>
        static Result_ MethodWrapper(void* object, Args_... args)
        {
                return (static_cast<Class_*>(object)->*Func_)(args...);
        }
};

template <typename Result_, typename Registrar_, typename Class_, typename ...Args_>
struct LuaBaseCallWrapper
{
        template <typename Registrar_::Func Func_>
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
};

template <typename Result_, typename Registrar_, typename Class_, typename ...Args_>
struct LuaCallWrapper
{
        using Base_ = LuaBaseCallWrapper<Result_, Registrar_, Class_, Args_...>;
        template <typename Registrar_::Func Func_>
        static int Call(lua_State* L)
        {
                Class_* obj = Base_::template CommonWork<Func_>(L);

                int i = lua_gettop(L); REM_UNUSED_WARNING(i); // looping variable that represents that stack index to extract a parameter from.
                return LuaReturn(L, Registrar_::template MethodWrapper<Func_>(obj, LuaExtract<Args_>(L, i)...));
       }
};

template <typename Registrar_, typename Class_, typename ...Args_>
struct LuaCallWrapper<void, Registrar_, Class_, Args_...>
{
        using Base_ = LuaBaseCallWrapper<void, Registrar_, Class_, Args_...>;
        template <typename Registrar_::Func Func_>
        static int Call(lua_State* L)
        {
                Class_* obj = Base_::template CommonWork<Func_>(L);

                int i = lua_gettop(L); REM_UNUSED_WARNING(i); // looping variable that represents that stack index to extract a parameter from.
                Registrar_::template MethodWrapper<Func_>(obj, LuaExtract<Args_>(L, i)...);
                return 0;
        }
};

} // luaglue

#undef REM_UNUSED_WARNING

#endif // LUA_METHOD_HPP