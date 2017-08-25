#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#define LUA_COMPAT_APIINTCASTS
#include <lua.hpp>

//! Force inline macro for all of the small functions
//! that would make debug builds with no inlining substantially slower.
//!
#if defined(__MSC_VER)
    #define LG_FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
    #define LG_FORCE_INLINE inline __attribute__((always_inline))
#else
    #define LG_FORCE_INLINE inline
#endif


namespace lg
{
// cstdint is more code than this entire library...
using ApiId = unsigned char;
using TypeId = unsigned short;

namespace detail
{

// TODO

template <ApiId ApiId_, TypeId TypeId_>
constexpr void* metatable_registry_index() { return (void*)1; }

template <ApiId ApiId_, TypeId TypeId_>
LG_FORCE_INLINE void push_metatable_of(lua_State*) {}

} // namespace detail

} // namespace lg


#endif // LG_COMMON_HPP
