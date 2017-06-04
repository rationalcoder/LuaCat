#ifndef LG_STACK_HPP
#define LG_STACK_HPP
#include <cstdint>
#include "lg_common.hpp"
#include "lg_utility.hpp"

//! \file lua_stack.hpp
//! \brief Handles interaction with the lua stack.
//!

namespace lg
{
namespace detail
{

//! Generates functions to push and extract types from the lua stack.
//!
//! \tparam T_ The type that needs to be pushed/extracted from the stack.
//! \tparam Args_ Template parameter pack (unused at call site) used
//! to allow for specializations of the form:
//! \code
//! template <typename ...Args_>
//! StackManager<Aggregate<Args_...>> {};
//! \endcode
//! \note If a stack manager for a needed type isn't in this file, you will
//! get a failed static_assert and will need to implement it yourself.
//!
template <typename T_, typename... Args_>
struct StackManager
{
    static_assert(detail::TypeDependentFalse<T_>::value, "No Lua StackManager defined for one or more of your types. See the README for more info.");

    //! Pushes a value onto the lua stack.
    //!
    //! \param L The lua state to use.
    //! \param val The value that you want to push.
    //! \returns How many values were pushed.
    //!
    static int push(lua_State* L, T_ val);

    //! Extracts a value from the lua stack.
    //!
    //! \tparam Index_ the index into the Lua stack to extract from.
    //! \param L The lua state that we are working with.
    //! \returns The converted value.
    //!
    template <std::size_t Index_>
    static T_ at(lua_State* L);
};


//! The implementation of any stack manager that works on signed integers.
//!
//! \note The implementation is provided through inheritance.
//!
template <typename T_>
struct SignedIntegerManager
{
    static int push(lua_State* L, T_ val)
    {
        lua_pushinteger(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static T_ at(lua_State* L)
    {
        return (T_)luaL_checkinteger(L, Index_);
    }
};


//! The implementation of any stack manager that works on unsigned integers.
//!
//! \note The implementation is provided through inheritance of this class.
//!
template <typename T_>
struct UnsignedIntegerManager
{
    static int push(lua_State* L, T_ val)
    {
        lua_pushunsigned(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static T_ at(lua_State* L)
    {
        return (T_)luaL_checkunsigned(L, Index_);
    }
};


//! The implementation of the float/double stack managers.
//!
//! \note The implementation is provided through inheritance of this class.
//!
template <typename T_>
struct RealNumberManager
{
    static int push(lua_State* L, T_ val)
    {
        lua_pushnumber(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static T_ at(lua_State* L)
    {
        return (T_)luaL_checknumber(L, Index_);
    }
};

//! Stack manager for booleans.
template <>
struct StackManager<bool>
{
    static int push(lua_State* L, bool val)
    {
        lua_pushboolean(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static bool at(lua_State* L)
    {
        return (bool)lua_toboolean(L, Index_);
    }
};

// Real number managers
template <>
struct StackManager<double> : RealNumberManager<double> {};
template <>
struct StackManager<float> : RealNumberManager<float> {};

// Signed int managers
template<>
struct StackManager<int8_t> : SignedIntegerManager<int8_t> {};
template<>
struct StackManager<int16_t> : SignedIntegerManager<int16_t> {};
template<>
struct StackManager<int32_t> : SignedIntegerManager<int32_t> {};
template<>
struct StackManager<int64_t> : SignedIntegerManager<int64_t> {};

// Unsigned int managers
template<>
struct StackManager<uint8_t> : UnsignedIntegerManager<uint8_t> {};
template<>
struct StackManager<uint16_t> : UnsignedIntegerManager<uint16_t> {};
template<>
struct StackManager<uint32_t> : UnsignedIntegerManager<uint32_t> {};
template<>
struct StackManager<uint64_t> : UnsignedIntegerManager<uint64_t> {};

} // end detail

} // end lg

#endif // LG_MANIP_HPP
