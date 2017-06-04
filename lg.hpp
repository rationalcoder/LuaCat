#ifndef LG_HPP
#define LG_HPP

#include <vector>
#include <cassert>
#include <tuple>
#include "lg_stack.hpp"

#define LG_METHOD(name, ptr) lg::Method<decltype(ptr), ptr>(name)

namespace lg
{

namespace detail
{

template <uint32_t ApiId_,
          uint32_t ClassId_,
          typename Class_,
          typename Pointer_,
          size_t NumArgs_>
struct MethodCallWrapperBase
{
    using Pointer = Pointer_;
    static constexpr size_t num_expanded_args() { return NumArgs_ + 1; } // +1 for the instance.

    //! Grabs the instance pointer and does common error checking.
    static Class_* instance(lua_State* L)
    {
        size_t numArgs = lua_gettop(L);
        if (numArgs != num_expanded_args()) luaL_error(L, "expected %d arguments, got %d", num_expanded_args(), numArgs);
        if (!lua_istable(L, 1)) luaL_argerror(L, 1, "expected instance. Did you forget to call with ':'?");
        if (lua_getfield(L, 1, "_instance") != LUA_TLIGHTUSERDATA) luaL_argerror(L, 1, "invalid instance.");
        Class_* obj = (Class_*)lua_touserdata(L, -1);

        if (lua_getfield(L, 1, "_api_id") != LUA_TNUMBER || lua_tointeger(L, -1) != ApiId_)
            luaL_argerror(L, 1, "invalid instance.");

        if (lua_getfield(L, 1, "_class_id") != LUA_TNUMBER || lua_tointeger(L, -1) != ClassId_)
            luaL_argerror(L, 1, "invalid instance.");

        // [1]: instance table
        // [2]: instance
        // [3]: API ID
        // [4]: Class ID
        lua_pop(L, 3);
        // [1]: instance table
        return obj;
    }
};

template <uint32_t ApiId_,
          uint32_t ClassId_,
          typename TypeSet_,
          typename Result_,
          typename Class_,
          typename... Args_>
struct MethodCallWrapper : MethodCallWrapperBase<ApiId_, ClassId_, Class_, Result_(Class_::*)(Args_...), sizeof...(Args_)>
{
    using Base = MethodCallWrapperBase<ApiId_, ClassId_, Class_, Result_(Class_::*)(Args_...), sizeof...(Args_)>;

    template <typename Base::Pointer Func_>
    static int call(lua_State* L)
    {
        return call_impl<Func_>(L, typename detail::BuildIndexSequence<sizeof...(Args_)>::Type{});
    }

    template <typename Base::Pointer Func_, std::size_t... Indices_>
    static int call_impl(lua_State* L, detail::IndexSequence<Indices_...>)
    {
        Class_* instance = Base::instance(L);
        return StackManager<Result_>::push(L, (instance->*Func_)(detail::StackManager<Args_>::at<Indices_ + 2>(L)...));
    }
};

template <uint32_t ApiId_,
          uint32_t ClassId_,
          typename TypeSet_,
          typename Class_,
          typename... Args_>
struct MethodCallWrapper<ApiId_, ClassId_, TypeSet_, void, Class_, Args_...>
       : MethodCallWrapperBase<ApiId_, ClassId_, Class_, void(Class_::*)(Args_...), sizeof...(Args_)>
{
    using Base = MethodCallWrapperBase<ApiId_, ClassId_, Class_, void(Class_::*)(Args_...), sizeof...(Args_)>;

    template <typename Base::Pointer Func_>
    static int call(lua_State* L)
    {
        call_impl<Func_>(L, typename detail::BuildIndexSequence<sizeof...(Args_)>::Type{});
        return 0;
    }

    template <typename Base::Pointer Func_, std::size_t... Indices_>
    static void call_impl(lua_State* L, detail::IndexSequence<Indices_...>)
    {
        Class_* instance = Base::instance(L);
        (instance->*Func_)(detail::StackManager<Args_>::at<Indices_ + 2>(L)...);
    }
};


template <uint32_t ApiId_,
          uint32_t ClassId_,
          typename TypeSet_,
          typename Result_,
          typename Class_,
          typename... Args_>
auto make_call_wrapper(Result_(Class_::*)(Args_...)) -> MethodCallWrapper<ApiId_, ClassId_, TypeSet_, Result_, Class_, Args_...>
{
    return MethodCallWrapper<ApiId_, ClassId_, TypeSet_, Result_, Class_, Args_...>{};
}

} // namespace detail

template <typename PointerType_, PointerType_ Pointer_>
class Method
{
public:
    Method(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

    template <uint32_t ApiId_, uint32_t ClassId_, typename TypeSet_>
    static void export_to(lua_State* L, char const* name)
    {
        using Wrapper = decltype(detail::make_call_wrapper<ApiId_, ClassId_, TypeSet_>(Pointer_));

        // Add the function to whatever table is on top of the stack.
        lua_pushcfunction(L, &Wrapper::template call<Pointer_>);
        printf("setting method: %s %d\n", name, lua_gettop(L));
        lua_setfield(L, -2, name);
    }

private:
    char const* name_;
};

namespace detail
{

template <typename ExportFunc_>
struct ExportPair
{
    ExportPair()
        : name(), exporter()
    {}

    ExportPair(char const* name, ExportFunc_ func)
        : name(name), exporter(func)
    {}

    bool empty() const { return !name || !exporter; }

    char const* name;
    ExportFunc_ exporter;

};

using MethodExportPair = ExportPair<void(*)(lua_State*, char const*)>;

} // namespace detail

struct Factory;

template <class Type_,
          class Factory_ = Factory>
class Class
{
public:
    using Type = Type_;
    using Factory = Factory_;

public:
    Class(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

private:
    char const* name_;
};

template <typename... Args_>
struct Constructor {};

template <class Type_,
          class Factory_ = Factory>
class Enum
{
public:
    using Type = Type_;
    using Factory = Factory_;

public:
    Enum(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

private:
    char const* name_;
};

template <typename T_>
struct EnumValue
{
    char const* name;
    T_ value;
};

template <uint32_t ApiId_,
          uint32_t TypeId_,
          typename Type_,
          typename Factory_,
          typename TypeSet_,
          typename TypeWrapper_>
class TypeExporter
{
    static_assert(detail::TypeDependentFalse<TypeWrapper_>::value,
    "\n\n(LG): No TypeExporter specialization found for that type wrapper. Check your call to lg::Api::set_types(). \n\n");
};

template <uint32_t ApiId_,
          uint32_t TypeId_,
          typename Type_,
          typename Factory_,
          typename TypeSet_>
class TypeExporter<ApiId_, TypeId_, Type_, Factory_, TypeSet_, lg::Class<Type_, Factory_>>
{
public:
    using Type = Type_;
    using Factory = Factory_;
    using Wrapper = lg::Class<Type_, Factory_>;
    using TypeSet = TypeSet_;

    static constexpr uint32_t api_id() { return ApiId_; }
    static constexpr uint32_t type_id() { return TypeId_; }

private:
    using MethodExportFunc = void(*)(lua_State* L, char const* name);
    using CtorExportFunc = void(*)(lua_State* L);

    template <typename... Args_>
    struct CtorExporter
    {
        static void export_to(lua_State* L)
        {
        }
    };

    struct OperatorExporter
    {
        //! Exports all available operators to the metatable on
        //! \param L The lua_State to export to.
        //! the top of the stack.
        //!
        static void export_to(lua_State* L)
        {
        }
    };

public:
    TypeExporter(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

    template <typename... Args_>
    void set_constructor(lg::Constructor<Args_...> ctor)
    {
        ctorExportFunc_ = &CtorExporter<Args_...>::export_to;
    }

    template <typename... Methods_>
    void add_methods(Methods_... method)
    {
        methodExportPairs_.reserve(sizeof...(Methods_));

        using Expand = int[];
        Expand{(methodExportPairs_.emplace_back(method.name(), method.template export_to<ApiId_, TypeId_, TypeSet>), 0)...};
    }

    void export_to(lua_State* L) const
    {
        printf("exporting class: %s\n", name_);
        // TODO: handle this better.
        assert(name_ && *name_ && "Attempted to export a class without a name.");
        assert(ctorExportFunc_ && "Attempted to export a class without a constructor.");

        // [1]: API table
        printf("sse %d=1\n", lua_gettop(L));
        lua_newtable(L); // Class table
        lua_newtable(L); // Metatable for the class table (need to set up the ctor)

        // [1]: API table
        // [2]: class table
        // [3]: class metatable
        printf("sse %d=3\n", lua_gettop(L));
        ctorExportFunc_(L); // Export the constructor (added to the class metatable).
        lua_setmetatable(L, -2); // Done with the class metatable, set it.
        lua_newtable(L); // Metatable for class instances.

        // [1]: API table
        // [2]: class table
        // [3]: instance metatable
        printf("sse %d=3\n", lua_gettop(L));
        OperatorExporter::export_to(L); // Export any available operators to the instance metatable.
        lua_setfield(L, -2, "_instance_mt");

        lua_newtable(L); // methods table
        for (const detail::MethodExportPair& p : methodExportPairs_)
            p.exporter(L, p.name);

        // [1]: API table
        // [2]: class table
        // [3]: methods table
        printf("sse %d=3\n", lua_gettop(L));
        lua_setfield(L, -2, "_methods");
        printf("sse %d=2\n", lua_gettop(L));
        lua_setfield(L, -2, name_);
        printf("sse %d=1\n", lua_gettop(L));

        // [1]: API table.
        printf("end of export\n");
        printf("sse %d=1\n", lua_gettop(L));
    }

private:
    char const* name_;
    CtorExportFunc ctorExportFunc_;
    std::vector<detail::MethodExportPair> methodExportPairs_;
};

template <uint32_t ApiId_,
          uint32_t TypeId_,
          typename Type_,
          typename Factory_,
          typename TypeSet_>
class TypeExporter<ApiId_, TypeId_, Type_, Factory_, TypeSet_, lg::Enum<Type_, Factory_>>
{
public:
    using Type = Type_;
    using Factory = Factory_;
    using Wrapper = lg::Enum<Type_, Factory_>;

    static constexpr uint32_t api_id() { return ApiId_; }
    static constexpr uint32_t type_id() { return TypeId_; }

public:
    TypeExporter(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

    template <typename... EnumValues_>
    void add_values(EnumValues_... values)
    {
    }

    void export_to(lua_State* L) const
    {
        printf("Exporting enum: %s\n", name_);
    }

private:
    char const* name_;
};


namespace detail
{

// Hurts to make a virtual base, but it's easy and one virtual call
// during an export isn't going to hurt anyone.
class ExporterSetBase
{
public:
    virtual void export_to(lua_State* L) = 0;
    virtual ~ExporterSetBase() {}
};

} // namespace detail

namespace detail
{

template <int Index_>
struct ExporterCaller
{
    template <typename Tuple_>
    static void export_to(const Tuple_& t, lua_State* L)
    {
        std::get<Index_>(t).export_to(L);
        ExporterCaller<Index_-1>::export_to(t, L);
    }
};

template <>
struct ExporterCaller<0>
{
    template <typename Tuple_>
    static void export_to(const Tuple_& t, lua_State* L)
    {
        std::get<0>(t).export_to(L);
    }
};

} // namespace detail

template <typename... TypeExporters_>
class ExporterSet final: public detail::ExporterSetBase
{
private:
    template <typename Type_>
    using ExporterFor = typename detail::TypeFinder<Type_, TypeExporters_...>::Type;

    using TypeSet = detail::TypeSet<typename TypeExporters_::Type...>;

public:
    ExporterSet(std::tuple<TypeExporters_...>&& exporters)
        : exporters_(exporters)
    {}

    ExporterSet(const ExporterSet&) = delete;

    template <class Type_>
    auto at() -> TypeExporter<ExporterFor<Type_>::api_id(),
                              ExporterFor<Type_>::type_id(),
                              typename ExporterFor<Type_>::Type,
                              typename ExporterFor<Type_>::Factory, TypeSet,
                              typename ExporterFor<Type_>::Wrapper>&
    {
        // Class IDs are just indices in an API's type set.
        return std::get<ExporterFor<Type_>::type_id()>(exporters_);
    }

    virtual void export_to(lua_State* L)
    {
        printf("Start of export types\n");
        detail::ExporterCaller<sizeof...(TypeExporters_)-1>::export_to(exporters_, L);
        printf("end of export types\n");
    }

private:
    std::tuple<TypeExporters_...> exporters_;
};


template <uint32_t ApiId_>
class Api
{
public:
    Api(char const* name)
        : name_(name), exporterSet_(nullptr)
    {}

    ~Api()
    {
        delete exporterSet_;
        exporterSet_ = nullptr;
    }

    Api(const Api&) = delete;

    Api(Api&& other)
        : name_(other.name_), exporterSet_(other.exporterSet_)
    {}

    template <typename... Wrappers_>
    auto set_types(Wrappers_... wrappers) -> ExporterSet<TypeExporter<ApiId_, detail::IndexOf<Wrappers_, Wrappers_...>::value,
                                                                      typename Wrappers_::Type,
                                                                      typename Wrappers_::Factory,
                                                                      detail::TypeSet<typename Wrappers_::Type...>,
                                                                      Wrappers_>...>&
    {
        delete exporterSet_;

        auto exporterTuple = std::make_tuple(TypeExporter<ApiId_, detail::IndexOf<Wrappers_, Wrappers_...>::value,
                                                          typename Wrappers_::Type,
                                                          typename Wrappers_::Factory,
                                                          detail::TypeSet<typename Wrappers_::Type...>,
                                                          Wrappers_>(wrappers.name())...);

        // This hurts, but it is necessary unless there is a substantial refactoring.
        auto* temp = new ExporterSet<TypeExporter<ApiId_, detail::IndexOf<Wrappers_, Wrappers_...>::value,
                                      typename Wrappers_::Type,
                                      typename Wrappers_::Factory,
                                      detail::TypeSet<typename Wrappers_::Type...>,
                                      Wrappers_>...>(std::move(exporterTuple));
        exporterSet_ = temp;
        return *temp;
    }

    void export_to(lua_State* L)
    {
        // If the API is named, push a new table. Otherwise, just use the global table.
        bool named = name_ && *name_;
        if (named) lua_newtable(L);
        else lua_pushglobaltable(L);

        printf("In Api. Stack size before exporting types: %d\n", lua_gettop(L));
        exporterSet_->export_to(L);
        printf("After export API\n");

        if (named) lua_setglobal(L, name_);
        else lua_pop(L, 1);
        printf("After export API all\n");
    }

private:
    char const* name_;
    detail::ExporterSetBase* exporterSet_;
};


template <unsigned Id_>
struct UniqueId {};

template <unsigned Id_>
auto make_api(const char* name, UniqueId<Id_>) -> Api<Id_>
{
    return Api<Id_>(name);
}

template <unsigned Id_>
auto make_id() -> UniqueId<Id_>
{
    return UniqueId<Id_>{};
}

template <typename T_>
auto enum_value(char const* name, T_ value) -> lg::EnumValue<T_>
{
    return lg::EnumValue<T_>{name, value};
}

} // namespace lg

#endif // LG_HPP
