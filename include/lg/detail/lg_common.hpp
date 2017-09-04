#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#include <cassert>
#include <cstdint>

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

using ApiId = uint8_t;
using TypeId = uint16_t;

// uint8_t isn't guaranteed to be a char type, making type punning to it UB.
// Does this matter practically? No. Will I do it anyway? Yes...
using Byte = unsigned char;

} // namespace lg


#endif // LG_COMMON_HPP
