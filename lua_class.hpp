#ifndef LUA_CLASS_HPP
#define LUA_CLASS_HPP
#include <vector>
#include <tuple>
#include <functional>
#include "lua_method_registrar.hpp"

//TODO: finish getting clean method call syntax by passing tables around.

namespace luaglue
{

template <typename _Class, typename ..._Args>
struct DefaultClassFactory
{
        static _Class* Create(_Args... args)
        {
                return new _Class(args...);
        }

        static void Destroy(_Class* obj)
        {
                delete obj;
                obj = nullptr;
        }
};

template <typename _Class, typename _ClassFactory, typename ..._Args>
class LuaClass
{
        enum { NUM_ARGS = std::tuple_size<std::tuple<_Args...>>::value };
        typedef bool(*RegisterFunc)(lua_State*, char const* className, char const* methodName);
public:
        LuaClass(char const* className)
                : className_(className)
        {}

        template<typename _Result, typename ..._MethodArgs>
        luaglue::LuaMethodRegistrar<_Result, _Class, _MethodArgs...> GetMethodRegistrar(_Result (_Class::*Func)(_MethodArgs...))
        {
                (void)Func; // unused parameter used for type deduction.
                return luaglue::LuaMethodRegistrar<_Result, _Class, _MethodArgs...>();
        }

        void AddRegisterFunc(char const* methodName, RegisterFunc func)
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
                if(!lua_istable(L, 1))
                {
                        luaL_error(L, "expected class table as first parameter; did you forget the ':' in ClassName:func() ?");
                        return 0;
                }
                lua_getfield(L, 1, "return_table");

                int firstParam = lua_gettop(L)-1;
                lua_pushlightuserdata(L, _ClassFactory::Create(LuaExtract<_Args>(L, firstParam)...));
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
                _Class* obj = (_Class*)lua_touserdata(L, -1);
                assert(obj);
                _ClassFactory::Destroy(obj);
                return 0;
        }

private:
        std::vector<std::tuple<char const*, RegisterFunc>> namedRegistrars_;
        char const* className_;
};

} // luaglue

// temporary working AddMethod implemetation.
#define AddMethod(inst, str, memFun) inst.AddRegisterFunc(str, (decltype(inst.GetMethodRegistrar(memFun))::GetWrapper<memFun>()))


#endif // LUA_CLASS_HPP
