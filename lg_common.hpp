#ifndef LG_COMMON_HPP
#define LG_COMMON_HPP

#include <cstdlib> // size_t
#include <lua.hpp>

namespace luaglue
{

namespace detail
{

// Helper index sequence. (pre c++14 library)

template<std::size_t... Indices_>
struct IndexSequence final
{
    typedef IndexSequence<Indices_..., sizeof...(Indices_)> Next;
};

// Builds an IndexSequence<0, 1, 2, ..., Num_-1>.
template<std::size_t Num_>
struct BuildIndexSequence final
{
    typedef typename BuildIndexSequence<Num_ - 1>::Type::Next Type;
};

template<>
struct BuildIndexSequence<0>
{
    typedef IndexSequence<> Type;
};

} // detail

} // luaglue

#endif // LG_COMMON_HPP
