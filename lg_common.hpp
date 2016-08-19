#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#include <cstdlib>
#include <lua.hpp>

namespace luaglue
{

namespace impl
{

typedef bool(*MethodRegistrar)(lua_State*, char const* className, char const* methodName);

template<std::size_t... Indices_>
struct IndexSequence
{
    typedef IndexSequence<Indices_..., sizeof...(Indices_)> Next;
};

// Builds an _Index_tuple<0, 1, 2, ..., _Num-1>.
template<std::size_t Num_>
struct BuildIndexSequence
{
    typedef typename BuildIndexSequence<Num_ - 1>::Type::Next Type;
};

template<>
struct BuildIndexSequence<0>
{
    typedef IndexSequence<> Type;
};

} // impl

} // luaglue

#endif // LG_COMMON_HPP
