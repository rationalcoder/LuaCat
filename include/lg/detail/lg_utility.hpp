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

template <typename Target_, typename Head_, class... Tail_>
struct TypeFinder<Target_, Head_, Tail_...>
{
    using Type = typename std::conditional<std::is_same<Target_, typename Head_::Type>::value,
                 Head_, typename TypeFinder<Target_, Tail_...>::Type>::type;
};


template <typename, typename...>
struct TypeListContains;

template <class Target_>
struct TypeListContains<Target_> : std::false_type {};

template <typename Target_, typename Head_, class... Tail_>
struct TypeListContains<Target_, Head_, Tail_...> : std::conditional<std::is_same<Target_, Head_>::value,
                                                                     std::true_type,
                                                                     TypeListContains<Head_, Tail_...>>::type {};

template <typename, typename...>
struct TypeListCountIf;

template <typename Predicate_>
struct TypeListCountIf<Predicate_> : std::integral_constant<std::size_t, 0> {};

template <typename Predicate_, typename Head_, typename... Tail_>
struct TypeListCountIf<Predicate_, Head_, Tail_...> :
        std::integral_constant<std::size_t, (std::size_t)Predicate_::template satisfied<Head_>()
                                            + TypeListCountIf<Predicate_, Tail_...>::value> {};


// The IndexSequence above is meant to implement the C++14 type.
// This one is for arbitrary indices, without the Next typedef.
template <std::size_t... Indices_>
struct IndexList
{
    template <bool Cond_, std::size_t Tail_>
    using AppendedIf = typename std::conditional<Cond_, IndexList<Indices_..., Tail_>,
                                                        IndexList<Indices_...>>::type;
};

template <typename, typename...>
struct TypeListIndicesMatching;

template <typename TypeList_, typename Predicate_>
struct TypeListIndicesMatching<TypeList_, Predicate_> { using List = IndexList<>; };

template <typename TypeList_, typename Predicate_, typename Head_, typename... Tail_>
struct TypeListIndicesMatching <TypeList_, Predicate_, Head_, Tail_...>
{
    using List = typename TypeListIndicesMatching<Predicate_, Tail_...>::List
                 ::template AppendedIf<Predicate_::template satisfied<Head_>(),
                                       TypeList_::template index_of<Head_>()>;
};


// Simple type list for storing raw user-defined types.

template <typename... Types_>
struct TypeList
{
    using This = TypeList<Types_...>;

    template <typename T_>
    static constexpr bool contains() { return TypeListContains<T_, Types_...>::value; }

    template <typename T_>
    static constexpr int index_of() { return IndexOf<T_, Types_...>::value; }

    static constexpr std::size_t size() { return sizeof...(Types_); }

    template <typename Predicate_>
    static constexpr std::size_t count_if() { return TypeListCountIf<Predicate_, Types_...>::value; }

    template <typename Predicate_>
    static constexpr auto matching_indices() -> typename TypeListIndicesMatching<This, Predicate_, Types_...>::List
    {
        return typename TypeListIndicesMatching<This, Predicate_, Types_...>::List{};
    }
};

template <typename Any_>
struct TypeDependentFalse : std::false_type {};


} // namespace detail
} // namespace lg

#endif // LG_UTILITY_HPP
