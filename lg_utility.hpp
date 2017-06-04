#ifndef LG_UTILITY_HPP
#define LG_UTILITY_HPP
#include <type_traits>

namespace lg
{
namespace detail
{

// Helper index sequence
template<std::size_t... Indices_>
struct IndexSequence
{
    using Next = IndexSequence<Indices_..., sizeof...(Indices_)>;
};

// Builds an IndexSequence<0, 1, 2, ..., Num_-1>.
template<std::size_t Num_>
struct BuildIndexSequence
{
    using Type = typename BuildIndexSequence<Num_ - 1>::Type::Next;
};

template<>
struct BuildIndexSequence<0>
{
    using Type = IndexSequence<>;
};

template <typename Target_, typename ...List_>
struct IndexOf : std::integral_constant<int, -1> {};

template <typename Target_, typename NotTarget_, typename ...List_>
struct IndexOf<Target_, NotTarget_, List_...> : std::integral_constant<int, 1 + IndexOf<Target_, List_...>::value> {};

template <typename Target_, typename ...List_>
struct IndexOf<Target_, Target_, List_...> : std::integral_constant<int, 0> {};


// Used for finding the type exporter in a type-list that corresponds to a user-defined type.

template <class, class...>
struct TypeFinder;

template <class Target_>
struct TypeFinder<Target_>
{
    using Type = void;
};

template <typename Target_, typename HeadType_, class... TailTypes_>
struct TypeFinder<Target_, HeadType_, TailTypes_...>
{
    using Type = typename std::conditional<std::is_same<Target_, typename HeadType_::Type>::value,
                 HeadType_, typename TypeFinder<Target_, TailTypes_...>::Type>::type;
};

// Simple type-set for storing raw user-defined types.

template <typename... Types_>
struct TypeSet
{
    static constexpr size_t size() { return sizeof...(Types_); }
};

template <typename Any_>
struct TypeDependentFalse : std::false_type {};


} // namespace detail
} // namespace lg

#endif // LG_UTILITY_HPP
