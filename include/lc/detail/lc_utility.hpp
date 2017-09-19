#ifndef LC_UTILITY_HPP
#define LC_UTILITY_HPP
#include <type_traits>
#include <lc/detail/lc_common.hpp>

namespace lc
{

// Meta-programming utilities.
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

// Defined later
template <typename... Types_>
struct TypeList;


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
struct TypeListIndicesMatching<TypeList_, Predicate_, Head_, Tail_...>
{
    using List = typename TypeListIndicesMatching<TypeList_, Predicate_, Tail_...>::List
                 ::template AppendedIf<Predicate_::template satisfied<Head_>(),
                                       TypeList_::template index_of<Head_>()>;
};

template <typename...>
struct TypeListFilter;

template <typename Predicate_>
struct TypeListFilter<Predicate_> { using List = TypeList<>; };

template <typename Predicate_, typename Head_, typename... Tail_>
struct TypeListFilter<Predicate_, Head_, Tail_...>
{
    using List = typename TypeListFilter<Predicate_, Tail_...>::List
                 ::template AppendedIf<Predicate_::template satisfied<Head_>(), Head_>;
};

// Simple type list for storing raw user-defined types.

template <typename... Types_>
struct TypeList
{
    using This = TypeList<Types_...>;

    template <bool Cond_, typename Tail_>
    using AppendedIf = typename std::conditional<Cond_, TypeList<Types_..., Tail_>,
                                                        TypeList<Types_...>>::type;
    template <typename T_>
    static constexpr bool contains() { return TypeListContains<T_, Types_...>::value; }

    static constexpr std::size_t size() { return sizeof...(Types_); }

    template <typename Predicate_>
    static constexpr std::size_t count_if() { return TypeListCountIf<Predicate_, Types_...>::value; }

    template <typename T_>
    static constexpr int index_of() { return IndexOf<T_, Types_...>::value; }

    template <typename T_, typename Predicate_>
    static constexpr int index_of_where() { return filter<Predicate_>().template index_of<T_>(); }

    template <typename Predicate_>
    static constexpr auto filter() -> typename TypeListFilter<Predicate_, Types_...>::List
    {
        return typename TypeListFilter<Predicate_, Types_...>::List{};
    }

    template <typename Predicate_>
    static constexpr auto matching_indices() -> typename TypeListIndicesMatching<This, Predicate_, Types_...>::List
    {
        return typename TypeListIndicesMatching<This, Predicate_, Types_...>::List{};
    }
};

struct HasMetatable
{
    template <typename T_>
    static constexpr bool satisfied() { return std::is_class<T_>::value; }
};

template <typename TypeList_, typename T_>
constexpr int metatable_index() { return TypeList_::template index_of_where<T_, HasMetatable>(); }

template <typename Any_>
struct TypeDependentFalse : std::false_type {};

} // namespace detail


// Containers
namespace detail
{

template <typename Size_, uint8_t Factor_>
struct ArrayTraits
{
    using Size = Size_;
    constexpr uint8_t factor() { return Factor_; }
};

using DefaultArrayTraits = ArrayTraits<int32_t, 2>;

// The default size is int32_t b/c you shouldn't be storing anything
// over uint16_t in a dynamic array. It takes std::size_t in its interface
// for consistency at call sites.
template <typename T_, typename ArrayTraits_ = DefaultArrayTraits>
class DynamicArray
{
public:
    using Traits = ArrayTraits_;

private:
    using Size = typename ArrayTraits_::Size;

public:
    DynamicArray(std::size_t startingSize = 0)
        : data_(nullptr), size_(0)
    {
        if (startingSize == 0) return;

        data_ = malloc(startingSize);
    }

    DynamicArray(DynamicArray& other)
    {
        Size size = other.size_;
        Size capactity = other.capacity_;

        size_ = size;
        capacity_ = capactity;
        data_ = malloc(capactity);

        T_* otherData = other.data_;
        for (Size i = 0; i < size; i++)
            data_[i] = otherData[i];
    }

    DynamicArray(DynamicArray&& other)
    {
        data_ = other.data_;
        capacity_ = other.capacity_;
        size_ = other.size_;

        other.data_ = nullptr;
    }

    ~DynamicArray() { free(data_); }

    DynamicArray& operator=(DynamicArray& other)
    {
        this->~DynamicArray();
        new (this) DynamicArray(other);
    }

    DynamicArray& operator=(DynamicArray&& other)
    {
        this->~DynamicArray();
        new (this) DynamicArray(other);
    }

    LC_FORCE_INLINE std::size_t size() const { return (std::size_t)size_; }

    LC_FORCE_INLINE T_* data() { return data_; }
    LC_FORCE_INLINE const T_* data() const { return data_; }
    LC_FORCE_INLINE const T_* const_data() const { return data_; }

    LC_FORCE_INLINE T_& operator[](std::size_t index) { return data_[index]; }
    LC_FORCE_INLINE const T_& operator[](std::size_t index) const { return data_[index]; }

    LC_FORCE_INLINE void reserve(std::size_t numElements)
    {
        Size requiredCapacity = (Size)(numElements * sizeof(T_));
        Size currentCapacity = capacity_;
        if (currentCapacity >= requiredCapacity) return;

        data_ = (T_*)realloc(data_, requiredCapacity);
        capacity_ = requiredCapacity;
    }

    LC_FORCE_INLINE void reserve_initialized(std::size_t numElements)
    {
        Size requiredCapacity = (Size)(numElements * sizeof(T_));
        Size currentCapacity = capacity_;
        if (currentCapacity >= requiredCapacity) return;

        data_ = (T_*)realloc(data_, requiredCapacity);
        capacity_ = requiredCapacity;

        Size oldSize = size_;
        size_ = numElements;
        for (Size i = 0; i < numElements - oldSize; i++)
            new (&data_[i]) T_();
    }

    LC_FORCE_INLINE void reserve_initialized(std::size_t numElements, const T_& val)
    {
        Size requiredCapacity = (Size)(numElements * sizeof(T_));
        Size currentCapacity = capacity_;
        if (currentCapacity >= requiredCapacity) return;

        data_ = (T_*)realloc(data_, requiredCapacity);
        capacity_ = requiredCapacity;

        Size oldSize = size_;
        size_ = numElements;
        for (Size i = 0; i < numElements - oldSize; i++)
            new (&data_[i]) T_(val);
    }

    LC_FORCE_INLINE void add_by_reference(const T_& t)
    {
        Size currentCapacity = capacity_;
        Size requiredCapacity = currentCapacity + sizeof(T_);
        if (requiredCapacity <= currentCapacity) {
            new (&data_[size_++]) T_(t);
            return;
        }

        data_ = realloc(data_, currentCapacity * ArrayTraits_::factor());
        new (&data_[size_++]) T_(t);
    }

    LC_FORCE_INLINE void add_by_reference(const T_&& t)
    {
        Size currentCapacity = capacity_;
        Size requiredCapacity = currentCapacity + sizeof(T_);
        if (requiredCapacity <= currentCapacity) {
            new (&data_[size_++]) T_(t);
            return;
        }

        data_ = realloc(data_, currentCapacity * ArrayTraits_::factor());
        new (&data_[size_++]) T_(t);
    }

    LC_FORCE_INLINE void add_by_value(T_ t)
    {
        Size currentCapacity = capacity_;
        Size requiredCapacity = currentCapacity + sizeof(T_);
        if (requiredCapacity <= currentCapacity) {
            data_[size_++] = t;
            return;
        }

        data_ = realloc(data_, currentCapacity * ArrayTraits_::factor());
        data_[size_++] = t;
    }

    template <typename... Args_>
    LC_FORCE_INLINE void emplace(Args_&&... args)
    {
        Size currentCapacity = capacity_;
        Size requiredCapacity = currentCapacity + sizeof(T_);
        if (requiredCapacity <= currentCapacity) {
            new (data_ + size_++) T_(args...);
            return;
        }

        data_ = realloc(data_, currentCapacity * ArrayTraits_::factor());
        new (data_ + size_++) T_(args...);
    }

private:
    T_* data_;
    Size size_;
    Size capacity_;
};

} // namespace detail
} // namespace lc

#endif // LC_UTILITY_HPP
