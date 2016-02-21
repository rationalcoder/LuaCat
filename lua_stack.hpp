#ifndef LUA_MANIP_HPP
#define LUA_MANIP_HPP
#include <lua5.2/lua.hpp>
#include <lua5.2/lauxlib.h>
#include <tuple>
#include <cstdint>

/**
 * @file lua_stack.hpp
 * @brief Handles interaction with the lua stack.
 */


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

/**
 * @brief Generates functions to push and pop types from the lua stack.
 * @tparam T_ The type that needs to be pushed/popped from the stack.
 * @tparam Args_ Template parameter pack (unused at call site) used
 * to allow for specializations of the form:
 * @code
 * template <typename ...Args_>
 * StackManager<Aggregate<Args_...>> {};
 * @endcode
 * @note If a stack manager for a needed type isn't in this file, you will
 * get a failed static_assert and will need to implement it yourself.
 */
template <typename T_, typename ...Args_>
struct StackManager
{
    /** Allow for a type-dependent static assertion that will only trigger at instantiation time. */
    template <typename Any_>
    struct False { enum { RESULT = false }; };

    static_assert(False<T_>::RESULT, "No Lua StackManager defined for one or more of your types. See the README for more info.");

    /**
     * @brief Pushes a value onto the lua stack.
     * @param L The lua state that we are working with.
     * @param val The value that we need to push.
     * @returns How many values were pushed.
     */
    static int Push(lua_State* L, T_ val);

    /**
     * @brief Pops a value from the lua stack.
     * @param L The lua state that we are working with.
     * @param firstParamIndex The current position in the stack
     * that we need to pop from.
     * @note Right now, all Pop functions need to manually decrement
     * firstParamIndex by the number of values they pop off the stack.
     * @returns The popped value.
     */
    static T_ Pop(lua_State* L, int* firstParamIndex);
};

/**
 * @brief The implementation of any stack manager that works on signed integers.
 * @note The implementation is provided through inheritance.
 */
template <typename T_>
struct SignedIntegerManager
{
    static int Push(lua_State* L, T_ val)
    {
        lua_pushinteger(L, val);
        return 1;
    }

    static T_ Pop(lua_State* L, int* firstParamIndex)
    {
        //TODO: Error Handling
        T_ result = lua_tointeger(L, (*firstParamIndex)--);
        return result;
    }
};

/**
 * @brief The implementation of any stack manager that works on unsigned integers.
 * @note The implementation is provided through inheritance.
 */
template <typename T_>
struct UnsignedIntegerManager
{
    static int Push(lua_State* L, T_ val)
    {
        lua_pushunsigned(L, val);
        return 1;
    }

    static T_ Pop(lua_State* L, int* firstParamIndex)
    {
        //TODO: Error Handling
        T_ result = lua_tounsigned(L, (*firstParamIndex)--);
        return result;
    }
};

/**
 * @brief The implementation of the float/double stack managers.
 * @note The implementation is provided through inheritance.
 */
template <typename T_>
struct RealNumberManager
{
    static int Push(lua_State* L, T_ val)
    {
        lua_pushnumber(L, val);
        return 1;
    }

    static T_ Pop(lua_State* L, int* firstParamIndex)
    {
        //TODO: Error Handling
        T_ result = lua_tonumber(L, (*firstParamIndex)--);
        return result;
    }
};

// boolean manager
template <>
struct StackManager<bool>
{
    static int Push(lua_State* L, bool val)
    {
        lua_pushboolean(L, val);
        return 1;
    }

    static bool Pop(lua_State* L, int* firstParamIndex)
    {
        //TODO: Error Handling
        bool result = lua_toboolean(L, (*firstParamIndex)--);
        return result;
    }
};

// Real number managers
template <>
struct StackManager<double> : public RealNumberManager<double> {};
template <>
struct StackManager<float> : public RealNumberManager<float> {};

// Signed int managers
template<>
struct StackManager<int8_t> : public SignedIntegerManager<int8_t> {};
template<>
struct StackManager<int16_t> : public SignedIntegerManager<int16_t> {};
template<>
struct StackManager<int32_t> : public SignedIntegerManager<int32_t> {};
template<>
struct StackManager<int64_t> : public SignedIntegerManager<int64_t> {};

// Unsigned int managers
template<>
struct StackManager<uint8_t> : public SignedIntegerManager<uint8_t> {};
template<>
struct StackManager<uint16_t> : public SignedIntegerManager<uint16_t> {};
template<>
struct StackManager<uint32_t> : public SignedIntegerManager<uint32_t> {};
template<>
struct StackManager<uint64_t> : public SignedIntegerManager<uint64_t> {};


} // end impl

} // end luaglue

#endif // LUA_MANIP_HPP
