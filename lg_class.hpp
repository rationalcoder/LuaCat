#ifndef LG_CLASS_HPP
#define LG_CLASS_HPP
#include <vector>
#include <tuple>
#include <functional>
#include "lg_common.hpp"
#include "lg_input.hpp"
#include "lg_method.hpp"

#define LG_METHOD(ptr) luaglue::MakeMethod(ptr).MakeRegistrar<ptr>()
#define LG_FUN(ptrl) luaglue::MakeFunction(ptr)

namespace luaglue 
{

template <typename Class_, typename ...Args_>
struct DefaultClassFactory final
{
    static Class_* Create(Args_... args) 
    {
        return new Class_(args...);
    }

    static void Destroy(Class_* obj) 
    {
        delete obj;
    }
};

template <typename Class_, typename ClassFactory_, typename ...Args_>
class LuaClass final
{
public:
    LuaClass(char const* className)
        : className_(className)
    {}

    void AddMethods(MethodSet description)
    {
        for (auto namedRegistrar : description.List())
            namedRegistrars_.push_back(namedRegistrar);
    }

    void Export(lua_State* L)  {
        // main creation and destruction functions
        luaL_Reg lib[] =
        {
            { "aquire", MakeCreationWrapper(typename detail::BuildIndexSequence<sizeof ...(Args_)>::Type()) },
            { "release", LuaDestructionWrapper },
            { NULL, NULL}
        };

        luaL_newlib(L, lib); // create the main table for this class: the one that has the "aquire" and "release" functions.
        lua_newtable(L); // create the table that will hold the methods and some meta-data of this class.
        lua_newtable(L); // create the meta-table that will be shared by all instances of the class.
        lua_pushcfunction(L, IndexMetaMethod); // push the c function used as our __index metamethod.
        // [-4]: Main table
        // [-3]: Methods and meta-data table
        // [-2]: Metatable
        // [-1]: Index meta-method c function
        lua_setfield(L, -2, "__index"); // set the __index meta-method in the meta table and pop the function from the stack.
        // [-3]: Main table
        // [-2]: Methods and meta-data table
        // [-1]: Metatable
        lua_setfield(L, -3, "_meta_table");
        // [-2]: Main table
        // [-1]: Methods and meta-data table
        lua_setfield(L, -2, "_impl_table"); // set the methods/meta-data table as a field of the main table.
        // [-1]: Main table
        lua_setglobal(L, className_); // set the main table as a new global with the name of the class, and pop it from the stack.
        printf("stack size: %d\n", lua_gettop(L));

        // call all of the register functions passing in the lua state.
        for (auto namedRegistrar : namedRegistrars_)
            namedRegistrar.registrar(L, className_, namedRegistrar.methodName);
    }

private:
    template <std::size_t ...Indices_>
    static constexpr lua_CFunction MakeCreationWrapper(detail::IndexSequence<Indices_...>) { return LuaCreationWrapper<Indices_...>; }

private:
    static int IndexMetaMethod(lua_State* L)
    {
        // [1]: object table
        // [2]: key (name of method to call)
        lua_getfield(L, 1, "_impl_table");
        // [1]: object table
        // [2]: key (name of method to call)
        // [-1]: impl table
        char const* keyName = lua_tostring(L, 2);
        // push the method on to the stack (or nil if not found)
        lua_getfield(L, -1, keyName);
        return 1;
    }
    
    template <std::size_t ...Indices_>
    static int LuaCreationWrapper(lua_State* L) {
        using namespace detail;

        // [1]: Class Table
        // [2...n]: "constructor" arguments
        if(!lua_istable(L, 1))
        {
            lua_pushnil(L);
            return 1;
        }

        // push the table the will represent the instance of this class type.
        lua_newtable(L);
        // [1]: class Table
        // [2...n]: "constructor" arguments
        // [-1]: object table
        // create the object and push a pointer to it on the stack.
        // Use Indices_ + 2: +1 to get 1 based indices, and +1 to start at index 2 (where the arguments start).
        lua_pushlightuserdata(L, ClassFactory_::Create(StackManager<Args_>::template At<Indices_ + 2>(L)...));
        // [1]: class Table
        // [2...n]: "constructor" arguments
        // [-2]: object table
        // [-1]: object pointer
        // set it as _intance in the new table and pop the pointer off the stack.
        lua_setfield(L, -2, "_instance");
        // [1]: class Table
        // [2...n]: "constructor" arguments
        // [-1]: object table
        // get the meta-table from the class table and set is as the meta-table for this object table.
        lua_getfield(L, 1, "_meta_table");
        // [1]: class Table
        // [2...n]: "constructor" arguments
        // [-2]: object table
        // [-1]: class meta-table
        lua_setmetatable(L, -2);
        // [1]: class Table
        // [2...n]: "constructor" arguments
        // [-1]: object table
        lua_getfield(L, 1, "_impl_table");
        // [1]: class Table
        // [2...n]: "constructor" arguments
        // [-2]: object table
        // [-1]: methods table
        // put a reference to the table with all the methods into the object table itself, for easier __index implementation.
        lua_setfield(L, -2, "_impl_table");
        return 1; // return the object table
    }

    static int LuaDestructionWrapper(lua_State* L)
    {
        if(!lua_istable(L, -1))
        {
            lua_pushnil(L);
            return 1;
        }

        lua_getfield(L, -1, "_instance");
        Class_* obj = (Class_*)lua_touserdata(L, -1);
        assert(obj);
        ClassFactory_::Destroy(obj);
        return 0;
    }

private:
    std::vector<detail::NamedMethodRegistrar> namedRegistrars_;
    char const* className_;
};

template<typename Result_, typename Class_, typename ...MethodArgs_>
constexpr LuaMethod<Result_, Class_, MethodArgs_...> MakeMethod(Result_ (Class_::*)(MethodArgs_...)) noexcept
{
    return LuaMethod<Result_, Class_, MethodArgs_...>();
}

} // luaglue

#endif // LG_CLASS_HPP
