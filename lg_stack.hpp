#ifndef LG_STACK_HPP
#define LG_STACK_HPP

#include <cstdint>
#include <type_traits>
#include "lg_common.hpp"
#include "lg_utility.hpp"

//! \file
//! \brief Handles interaction with the lua stack.
//!

namespace lg
{
namespace detail
{

//! Removes all pointers, references, and cv qualifiers from a type (not perfectly).
template <typename T_>
struct unqualified_type
{
    using type = typename std::remove_cv<
                 typename std::remove_reference<
                 typename std::remove_pointer<
                 typename std::remove_cv<T_>
                 ::type>::type>::type>::type;
};

//! Returns whether or not a type is a valid user type.
//! That is, a type that is either a type from the API type-list or
//! a reference or pointer to a type from the API type-list.
//!
//! \tparam T_ The type to check.
//! \tparam ApiTypeList_ The API type-list to check against.
//! \retval Defines a constexpr boolean, ::value, indicating the result.
//!
template <typename T_, typename ApiTypeList_>
struct is_user_type : std::conditional<ApiTypeList_::template contains<typename unqualified_type<T_>::type>(),
                                       std::false_type,
                                       std::true_type>::type {};

template <typename T_, typename ApiTypeList_, ApiId ApiId_>
struct UserTypeStackManager;

//! StackManager base for types that aren't valid user types for whatever reason.
//! This also serves as a good place to document the two functions that define a StackManager,
//! since they need to be present here to make sure that eroneous errors are deferred to link-time.
//!
template <typename T_>
struct UknownTypeStackManager
{
    // TODO: make two of these to report the different possible errors.
    static_assert(detail::TypeDependentFalse<T_>::value, "No Lua StackManager defined for one or more of your types.");

    //! Pushes a value onto the Lua stack.
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

//! Primary template; generates functions to push and extract types from the lua stack.
//!
template <typename T_, typename ApiTypeList_, ApiId ApiId_>
struct StackManager : std::conditional<is_user_type<T_, ApiTypeList_>::value,
                      UserTypeStackManager<T_, ApiTypeList_, ApiId_>,
                      UknownTypeStackManager<T_>>::type {};

//! Current representation of objects.
//! There will probably be support for different representations to avoid the
//! extra indirection, but this will work fine until the basic features are implemented.
//!
struct UserDataContents
{
    void* instance = nullptr;
    ApiId typeId = 0;
    TypeId apiId = 0;
};

inline char const* function_name(lua_State* L)
{
    lua_Debug debug;
    lua_getstack(L, 0, &debug);
    lua_getinfo(L, "n", &debug);

    return debug.name;
}


template <typename T_, typename ApiTypeList_, ApiId ApiId_>
struct UserTypeStackManager
{
    static LG_FORCE_INLINE int push(lua_State* L, T_* val)
    {
        UserDataContents* contents = (UserDataContents*)lua_newuserdata(L, sizeof(UserDataContents));
        contents->apiId = ApiId_;
        contents->typeId = IndexOf<T_, ApiTypeList_>::value;
        contents->instance = val;



        return 1;
    }

    template <std::size_t Index_>
    static LG_FORCE_INLINE T_* at(lua_State* L)
    {
        return nullptr;
    }
};

//! The implementation of any stack manager that works on signed integers.
//!
//! \note The implementation is provided through inheritance.
//!
template <typename T_>
struct SignedIntegerManager
{
    static LG_FORCE_INLINE int push(lua_State* L, T_ val)
    {
        lua_pushinteger(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static LG_FORCE_INLINE T_ at(lua_State* L)
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
    static LG_FORCE_INLINE int push(lua_State* L, T_ val)
    {
        lua_pushunsigned(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static LG_FORCE_INLINE T_ at(lua_State* L)
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
    static LG_FORCE_INLINE int push(lua_State* L, T_ val)
    {
        lua_pushnumber(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static LG_FORCE_INLINE T_ at(lua_State* L)
    {
        return (T_)luaL_checknumber(L, Index_);
    }
};

//! Stack manager for booleans.
template <typename ApiTypeList_, ApiId ApiId_>
struct StackManager<bool, ApiTypeList_, ApiId_>
{
    static LG_FORCE_INLINE int push(lua_State* L, bool val)
    {
        lua_pushboolean(L, val);
        return 1;
    }

    template <std::size_t Index_>
    static LG_FORCE_INLINE bool at(lua_State* L)
    {
        return (bool)lua_toboolean(L, Index_);
    }
};

// Real number managers
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<double, ApiTypeList_, ApiId_> : RealNumberManager<double> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<float, ApiTypeList_, ApiId_> : RealNumberManager<float> {};

// Signed int managers
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<int8_t, ApiTypeList_, ApiId_> : SignedIntegerManager<int8_t> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<int16_t, ApiTypeList_, ApiId_> : SignedIntegerManager<int16_t> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<int32_t, ApiTypeList_, ApiId_> : SignedIntegerManager<int32_t> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<int64_t, ApiTypeList_, ApiId_> : SignedIntegerManager<int64_t> {};

// Unsigned int managers
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<uint8_t, ApiTypeList_, ApiId_> : UnsignedIntegerManager<uint8_t> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<uint16_t, ApiTypeList_, ApiId_> : UnsignedIntegerManager<uint16_t> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<uint32_t, ApiTypeList_, ApiId_> : UnsignedIntegerManager<uint32_t> {};
template <typename ApiTypeList_, ApiId ApiId_> struct StackManager<uint64_t, ApiTypeList_, ApiId_> : UnsignedIntegerManager<uint64_t> {};

} // end detail
} // end lg

#endif // LG_MANIP_HPP
