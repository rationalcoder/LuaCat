#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#define LUA_COMPAT_APIINTCASTS
#include <lua.hpp>

//! Force inline macro for all of the small functions
//! that would make debug builds with no inlining substantially slower.
//!
#define LG_FORCE_INLINE __forceinline

namespace lg
{
// cstdint is more code than this entire library...
using ApiId = unsigned char;
using TypeId = unsigned short;

constexpr lua_Integer metatable_storage_index() { return 0xbabe; }

} // namespace lg


#endif // LG_COMMON_HPP
