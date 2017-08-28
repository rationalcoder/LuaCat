#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#include <cassert>
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
// Ideally, this is uint16_t. When it isn't, we can just static assert or something
// if the user manages to put more than UINT16_MAX types in an API...
using TypeId = unsigned short;

} // namespace lg


#endif // LG_COMMON_HPP
