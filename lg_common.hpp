#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#define LUA_COMPAT_APIINTCASTS
#include <lua.hpp>

namespace lg
{
// cstdint is more code than this entire library...
using ApiId = unsigned char;
using TypeId = unsigned short;
} // namespace lg

#endif // LG_COMMON_HPP
