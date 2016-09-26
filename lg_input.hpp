#ifndef LG_INPUT_HPP
#define LG_INPUT_HPP

#include <initializer_list>
#include "lg_common.hpp"

namespace luaglue
{

namespace detail
{

template <typename InitializerElem_>
class InputSet
{
public:
    constexpr InputSet(std::initializer_list<InitializerElem_> methods) noexcept
        : elems_(methods)
    {}

    constexpr const std::initializer_list<InitializerElem_>& List() const noexcept { return elems_; }

private:
    std::initializer_list<InitializerElem_> elems_;
};


using MethodRegistrar = bool(*)(lua_State* L, char const* className, char const* methodName);
struct NamedMethodRegistrar
{
    char const* methodName;
    MethodRegistrar registrar;
};

struct NamedFunction final
{
};

} // end detail

class MethodSet final : public detail::InputSet<detail::NamedMethodRegistrar>
{
    using detail::InputSet<detail::NamedMethodRegistrar>::InputSet;
};

class FunctionSet final
{

};

class OperatorSet final
{

};

} // end luaglue

#endif // LG_INPUT_HPP
