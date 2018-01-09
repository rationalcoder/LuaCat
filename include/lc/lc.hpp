#ifndef LC_HPP
#define LC_HPP

#include <vector>
#include <tuple>
#include <lc/detail/lc_stack.hpp>

#define LC_METHOD(name, ptr) lc::Method<decltype(ptr), ptr>(name)
// @Temporary until we replace vector?
#define LC_EXPAND_EMPLACE(vec, ...)\
do {\
    using Expand = int[];\
    Expand{(vec.emplace_back(__VA_ARGS__), 0)...};\
} while (false)

#define LC_EXPAND_PUSH_BACK(vec, expr)\
do {\
    using Expand = int[];\
    Expand{(vec.push_back(expr), 0)...};\
} while (false)


namespace lc
{

namespace detail
{

template <ApiId ApiId_,
          TypeId ClassId_,
          typename Class_,
          size_t NumArgs_>
struct MethodCallWrapperBase
{
    static constexpr size_t num_expanded_args() { return NumArgs_ + 1; } // +1 for the instance.

    //! Grabs the instance pointer and does common error checking.
    static Class_* instance(lua_State* L)
    {
        int numArgs = lua_gettop(L);
        if (numArgs != num_expanded_args()) luaL_error(L, "In function '%s': expected %d arguments(including self), got %d",
                                                       function_name(L), num_expanded_args(), numArgs);
        if (!lua_isuserdata(L, 1)) luaL_argerror(L, 1, "expected instance. Did you forget to call with ':'?");

        UserDataContents* contents = (UserDataContents*)lua_touserdata(L, 1);
        if (contents->apiId != ApiId_) luaL_argerror(L, 1, "invalid instance(bad API ID)");
        if (contents->typeId != ClassId_) luaL_argerror(L, 1, "invalid instance(bad type ID)");

        return (Class_*)contents->instance;
    }
};

template <ApiId ApiId_,
          TypeId ClassId_,
          typename TypeSet_,
          typename Result_,
          typename Class_,
          typename... Args_>
struct MethodCallWrapper : MethodCallWrapperBase<ApiId_, ClassId_, Class_, sizeof...(Args_)>
{
    using Base = MethodCallWrapperBase<ApiId_, ClassId_, Class_, sizeof...(Args_)>;
    using Pointer = Result_(Class_::*)(Args_...);
    using Class = Class_;
    using Result = Result_;

    // Note: we can't pass the pointer type to the base class, typedef it, and use it here
    // due to a bug in MSVC; you end up with "cannot convert overloaded-function to lua_CFunction" errors,
    // presumably b/c it's just too much type indirection for it to handle.

    template <Pointer Func_>
    static int call(lua_State* L)
    {
        return call_impl<Func_>(L, typename detail::BuildIndexSequence<sizeof...(Args_)>::Type{});
    }

    template <Result_(Class_::*Func_)(Args_...), std::size_t... Indices_>
    static int LC_FORCE_INLINE call_impl(lua_State* L, detail::IndexSequence<Indices_...>)
    {
        Class_* instance = Base::instance(L);
        return StackManager<Result_, TypeSet_, ApiId_>::push(L, (instance->*Func_)(
               detail::StackManager<Args_, TypeSet_, ApiId_>::template at<Indices_ + 2>(L)...));
    }
};

template <ApiId ApiId_,
          TypeId ClassId_,
          typename TypeSet_,
          typename Class_,
          typename... Args_>
struct MethodCallWrapper<ApiId_, ClassId_, TypeSet_, void, Class_, Args_...>
       : MethodCallWrapperBase<ApiId_, ClassId_, Class_, sizeof...(Args_)>
{
    using Base = MethodCallWrapperBase<ApiId_, ClassId_, Class_, sizeof...(Args_)>;
    using Pointer = void(Class_::*)(Args_...);
    using Class = Class_;
    using Result = void;

    template <Pointer Func_>
    static int call(lua_State* L)
    {
        call_impl<Func_>(L, typename detail::BuildIndexSequence<sizeof...(Args_)>::Type{});
        return 0;
    }

    template <Pointer Func_, std::size_t... Indices_>
    static LC_FORCE_INLINE void call_impl(lua_State* L, detail::IndexSequence<Indices_...>)
    {
        Class_* instance = Base::instance(L);
        (instance->*Func_)(detail::StackManager<Args_, TypeSet_, ApiId_>::template at<Indices_ + 2>(L)...);
    }
};

template <ApiId ApiId_,
          TypeId ClassId_,
          typename TypeSet_,
          typename Result_,
          typename Class_,
          typename... Args_>
auto make_call_wrapper(Result_(Class_::*)(Args_...)) -> MethodCallWrapper<ApiId_, ClassId_, TypeSet_, Result_, Class_, Args_...>
{
    return MethodCallWrapper<ApiId_, ClassId_, TypeSet_, Result_, Class_, Args_...>{};
}

} // namespace detail

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

using MethodExportPair = ExportPair<void(*)(lua_State*, char const*, lua_Integer*)>;

} // namespace detail

template <typename PointerType_, PointerType_ Pointer_>
class Method
{
public:
    explicit Method(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

    template <ApiId ApiId_, TypeId TypeId_, typename TypeSet_>
    static void export_to(lua_State* L, char const* name, lua_Integer* classMetatables)
    {
        using Wrapper = decltype(detail::make_call_wrapper<ApiId_, TypeId_, TypeSet_>(Pointer_));
        using Result = typename lc::detail::unqualified_type<typename Wrapper::Result>::type;
        // TODO: static_assert result is a pointer to an API type if result is not a value.

        // If the type doesn't have a metatable, just push and set the function as usual
        // in the table on top of the stack. This is expected to be resolved at compile-time.
        if (!lc::detail::HasMetatable<Result>::value) {
            lua_pushcfunction(L, &Wrapper::template call<Pointer_>);
            lua_setfield(L, -2, name);
            return;
        }

        // If, however, the type DOES have a metatable, the user type stack manager
        // expects the type's metatable as the function's first upvalue.
        lua_rawgeti(L, LUA_REGISTRYINDEX, classMetatables[lc::detail::metatable_index<TypeSet_, Result>()]);
        lua_pushcclosure(L, &Wrapper::template call<Pointer_>, 1);
        lua_setfield(L, -2, name);
    }

private:
    char const* name_;
};

struct NullFactory {};

template <typename T_, typename... CtorArgs_>
struct HeapFactory
{
    static LC_FORCE_INLINE T_* make(CtorArgs_... args) { return new T_(args...); }
    static LC_FORCE_INLINE void free(T_* p) { delete p; }
};

template <typename... Args_>
struct Constructor {};

using DefaultConstructor = Constructor<>;

template <class Type_,
          class Factory_ = lc::HeapFactory<Type_>>
class Class
{
public:
    using Type = Type_;
    using Factory = Factory_;

public:
    explicit Class(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

private:
    char const* name_;
};

template <class Type_,
          class Factory_ = NullFactory>
class Enum
{
public:
    using Type = Type_;
    using Factory = Factory_;

public:
    explicit Enum(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

private:
    char const* name_;
};

namespace detail
{
struct RawEnumValue 
{
    char const* name = nullptr; 
    lua_Integer value = 0;

    RawEnumValue() {}
    RawEnumValue(char const* name, lua_Integer value)
        : name(name), value(value)
    {}
};
} // namespace detail

// @Note: the cake is a lie. T_ is just for type-safety
// at call sites. We have to store lua_Integer's anyway,
// so it doesn't matter.
template <typename T_>
struct EnumValue : lc::detail::RawEnumValue
{
    EnumValue() {}
    EnumValue(char const* name, T_ value)
        : RawEnumValue(name, (lua_Integer)value)
    {}
};


namespace detail
{

enum SpecialKeys : lua_Integer
{
    INSTANCE_METATABLE,
    TYPE_NAMES
};

}

template <ApiId ApiId_,
          TypeId TypeId_,
          typename Type_,
          typename Factory_,
          typename TypeSet_,
          typename TypeWrapper_>
class TypeExporter
{
    static_assert(detail::TypeDependentFalse<TypeWrapper_>::value,
    "\n\n(LC): No TypeExporter specialization found for that type wrapper. Check your call to lc::Api::set_types(). \n\n");
};

//! Type exporter for classes.
template <ApiId ApiId_,
          TypeId TypeId_,
          typename Type_,
          typename Factory_,
          typename TypeSet_>
class TypeExporter<ApiId_, TypeId_, Type_, Factory_, TypeSet_, lc::Class<Type_, Factory_>>
{
public:
    using Type = Type_;
    using Factory = Factory_;
    using Wrapper = lc::Class<Type_, Factory_>;
    using TypeSet = TypeSet_;

    static constexpr ApiId api_id() { return ApiId_; }
    static constexpr TypeId type_id() { return TypeId_; }

private:
    using CtorExportFunc = void(*)(lua_State* L);

    template <typename... Args_>
    struct CtorExporter
    {
        static void export_to(lua_State* L)
        {
            // [-3]: instance metatable
            // [-2]: methods table
            // [-1]: class metatable
            lua_pushvalue(L, -3);
            lua_pushcclosure(L, &call_metamethod, 1);
            lua_setfield(L, -2, "__call");
            lua_pushvalue(L, -2);
            // [-4]: instance metatable
            // [-3]: methods table
            // [-2]: class metatable
            // [-1]: methods table (copy)
            lua_pushcclosure(L, &index_metamethod, 1);
            // [-4]: instance metatable
            // [-3]: methods table
            // [-2]: class metatable
            // [-1]: index metamethod
            lua_setfield(L, -4, "__index");
            lua_pushcfunction(L, &gc_metamethod);
            lua_setfield(L, -4, "__gc");
            // [-3..-1]: what we started with.
        }

        static int call_metamethod(lua_State* L)
        {
            return call_metamethod_impl(L, typename detail::BuildIndexSequence<sizeof...(Args_)>::Type{});
        }

        template <std::size_t... Indices_>
        static int call_metamethod_impl(lua_State* L, detail::IndexSequence<Indices_...>)
        {
            using Contents = lc::detail::UserDataContents;
            size_t numArgs = lua_gettop(L);
            if (numArgs != sizeof...(Args_) + 1) luaL_error(L, "In constructor for type '%s': expected %d arguments, got %d",
                                                            detail::function_name(L), sizeof...(Args_), numArgs-1);
            Type* instance = Factory::make(detail::StackManager<Args_, TypeSet_, ApiId_>::template at<Indices_ + 2>(L)...);
            if (!instance) return luaL_error(L, "Failed to allocate object(API ID: %u, Type ID: %u).", api_id(), type_id());

            Contents contents;
            contents.apiId = api_id();
            contents.typeId = type_id();
            contents.instance = instance;

            Contents* uData = (Contents*)lua_newuserdata(L, sizeof(contents));
            *uData = contents;

            // [1]: class table
            // [2]: new userdata
            // lua_rawgeti(L, 1, detail::SpecialKeys::INSTANCE_METATABLE);
            lua_pushvalue(L, lua_upvalueindex(1));
            lua_setmetatable(L, -2);
            // [1]: class table
            // [2]: new userdata
            return 1;
        }

        // @Optimization: the same one can be used for all classes.
        static int index_metamethod(lua_State* L)
        {
            // [1]: instance userdata
            // [2]: method name
            lua_pushvalue(L, lua_upvalueindex(1)); // push the _methods table
            lua_getfield(L, -1, lua_tostring(L, -2)); // get the method, or nil
            return 1;
        }

        static int gc_metamethod(lua_State* L)
        {
            using Contents = lc::detail::UserDataContents;
            Contents* contents = (Contents*)lua_touserdata(L, -1);
            Factory_::free((Type*)contents->instance);
            return 0;
        }
    };

    struct OperatorExporter
    {
        //! Exports all available operators to the metatable on top of the stack
        //! \param L The lua_State to export to.
        //!
        static void export_to(lua_State*)
        {
        }
    };

public:
    explicit TypeExporter(char const* name)
        : name_(name), methodsTable_(LUA_NOREF)
    {}

    char const* name() const { return name_; }

    template <typename... Args_>
    void set_constructor(lc::Constructor<Args_...>)
    {
        ctorExportFunc_ = &CtorExporter<Args_...>::export_to;
    }

    template <typename... Methods_>
    void add_methods(Methods_... methods)
    {
        methodExportPairs_.reserve(sizeof...(Methods_));
        LC_EXPAND_EMPLACE(methodExportPairs_, methods.name(),
                          methods.template export_to<ApiId_, TypeId_, TypeSet_>);
    }

    // During this phase, we are responsible for exporting our type to the lua state
    // along with type information (TODO) and storing our instance metatable in the list.
    void export_meta(lua_State* L, lua_Integer* classMetatables) const
    {
        // TODO: handle this better.
        assert(name_ && *name_ && "Attempted to export a class without a name.");
        assert(ctorExportFunc_ && "Attempted to export a class without a constructor.");

        // [1]: API table
        lua_newtable(L); // class table
        lua_newtable(L); // instance metatable.
        lua_newtable(L); // instance methods table
        lua_newtable(L); // class metatable

        // [1]: API table
        // [2]: class table
        // [3]: instance metatable
        // [4]: instance methods table
        // [5]: class metatable
        ctorExportFunc_(L); // Export the constructor (added to the class metatable).
        lua_setmetatable(L, 2); // Done with the class metatable, set it.

        // [1]: API table
        // [2]: class table
        // [3]: instance metatable
        // [4]: instance methods table
        methodsTable_ = luaL_ref(L, LUA_REGISTRYINDEX);
        // Export Lua compatible operators to the instance metatable.
        OperatorExporter::export_to(L);
        classMetatables[lc::detail::metatable_index<TypeSet, Type>()] = luaL_ref(L, LUA_REGISTRYINDEX);

        // [1]: API table
        // [2]: class table
        lua_setfield(L, -2, name_);

        // [1]: API table.
    }

    // Presumably, by the time this function is called, all of the metatables are filled out.
    void export_other(lua_State* L, lua_Integer* classMetatables) const
    {
        // [1]: API table.
        // Grab the methods table, fill it, and pop it back off.
        lua_rawgeti(L, LUA_REGISTRYINDEX, methodsTable_);
        for (const detail::MethodExportPair& p : methodExportPairs_)
            p.exporter(L, p.name, classMetatables);
        lua_pop(L, 1);
        // [1]: API table.
    }

private:
    char const* name_;
    CtorExportFunc ctorExportFunc_;
    std::vector<detail::MethodExportPair> methodExportPairs_;
    mutable lua_Integer methodsTable_;
};

//! Type exporter for enums.
template <ApiId ApiId_,
          TypeId TypeId_,
          typename Type_,
          typename Factory_,
          typename TypeSet_>
class TypeExporter<ApiId_, TypeId_, Type_, Factory_, TypeSet_, lc::Enum<Type_, Factory_>> 
{
public:
    using Type = Type_;
    using Factory = Factory_;
    using Wrapper = lc::Enum<Type_, Factory_>;

    static constexpr ApiId api_id() { return ApiId_; }
    static constexpr TypeId type_id() { return TypeId_; }

public:
    explicit TypeExporter(char const* name)
        : name_(name)
    {}

    char const* name() const { return name_; }

    template <typename... EnumValues_>
    void add_values(EnumValues_... values) 
    {
        LC_EXPAND_PUSH_BACK(values_, (lc::detail::RawEnumValue)values);
    }

    void export_meta(lua_State*, lua_Integer*) const {}
    void export_other(lua_State* L, lua_Integer*) const
    {
        // No point in exporting if there aren't any values...
        if (values_.empty()) return;

        // [1]: API table
        lua_newtable(L); // enum class table
        for (const lc::detail::RawEnumValue& v : values_) {
            using Contents = lc::detail::EnumClassContents;

            // @Space, @Slow. This is a wasteful implementation. This way, each stores
            // an api id and type id, when they could all reference the same ones.
            Contents* contents = (Contents*)lua_newuserdata(L, sizeof(Contents));
            contents->apiId = ApiId_;
            contents->typeId = TypeId_;
            contents->value = (lua_Integer)v.value;
            lua_setfield(L, -2, v.name);
        }
        lua_setfield(L, -2, name_);
    }

private:
    char const* name_;
    std::vector<lc::detail::RawEnumValue> values_;
};

namespace detail
{

template <std::size_t Index_>
struct ExporterCaller
{
    template <typename Tuple_>
    static LC_FORCE_INLINE void export_meta(const Tuple_& t, lua_State* L, lua_Integer* classMetatables)
    {
        std::get<Index_>(t).export_meta(L, classMetatables);
        ExporterCaller<Index_-1>::export_meta(t, L, classMetatables);
    }

    template <typename Tuple_>
    static LC_FORCE_INLINE void export_other(const Tuple_& t, lua_State* L, lua_Integer* classMetatables)
    {
        std::get<Index_>(t).export_other(L, classMetatables);
        ExporterCaller<Index_-1>::export_other(t, L, classMetatables);
    }
};

template <>
struct ExporterCaller<0>
{
    template <typename Tuple_>
    static LC_FORCE_INLINE void export_meta(const Tuple_& t, lua_State* L, lua_Integer* classMetatables)
    {
        std::get<0>(t).export_meta(L, classMetatables);
    }

    template <typename Tuple_>
    static LC_FORCE_INLINE void export_other(const Tuple_& t, lua_State* L, lua_Integer* classMetatables)
    {
        std::get<0>(t).export_other(L, classMetatables);
    }
};

struct ExporterSetWrapper
{
    void* exporterSet = nullptr;
    void (*exportFunc)(void*, lua_State*) = nullptr;
    void (*free)(void*) = nullptr;
    void export_to(lua_State* L) { exportFunc(exporterSet, L); }

    void delete_and_null()
    {
        if (exporterSet) {
            free(exporterSet);
            exporterSet = nullptr;
        }
    }
};

template <typename T_>
struct ExporterSetWrapperFactory
{
    static void export_to(void* p, lua_State* L) { ((T_*)p)->export_to(L); }
    static void free(void* p) { delete ((T_*)p); }
};

template <typename T_>
static lc::detail::ExporterSetWrapper wrap_exporter_set(T_* exporterSet)
{
    using Wrapper = lc::detail::ExporterSetWrapper;
    using Factory = lc::detail::ExporterSetWrapperFactory<T_>;

    Wrapper result;
    result.exporterSet = exporterSet;
    result.exportFunc = &Factory::export_to;
    result.free = &Factory::free;
    return result;
}

} // namespace detail

template <typename... TypeExporters_>
class ExporterSet
{
private:
    template <typename Type_>
    using ExporterFor = typename detail::TypeFinder<Type_, TypeExporters_...>::Type;
    using TypeSet = detail::TypeList<typename TypeExporters_::Type...>;

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

    void export_to(lua_State* L)
    {
        // There are two phases, "meta" and "other" so that all types exporters
        // can register their type names, metatables, etc. somewhere in the lua_State
        // before they register things like methods that depend on API type information
        // and metatables of other types in the API.
        detail::ExporterCaller<sizeof...(TypeExporters_)-1>::export_meta(exporters_, L, metatables_);
        detail::ExporterCaller<sizeof...(TypeExporters_)-1>::export_other(exporters_, L, metatables_);
    }

private:
    std::tuple<TypeExporters_...> exporters_;
    lua_Integer metatables_[TypeSet::template filter<lc::detail::HasMetatable>().size()];
};

template <ApiId ApiId_>
class Api
{
public:
    explicit Api(char const* name)
        : name_(name)
    {}

    ~Api()
    {
        exporterSet_.delete_and_null();
    }

    Api(const Api&) = delete;

    Api(Api&& other)
        : name_(other.name_), exporterSet_(other.exporterSet_)
    {}

    template <typename... Wrappers_>
    auto set_types(Wrappers_... wrappers) -> ExporterSet<TypeExporter<ApiId_, detail::IndexOf<Wrappers_, Wrappers_...>::value,
                                                                      typename Wrappers_::Type,
                                                                      typename Wrappers_::Factory,
                                                                      detail::TypeList<typename Wrappers_::Type...>,
                                                                      Wrappers_>...>&
    {
        exporterSet_.delete_and_null();

        auto exporterTuple = std::make_tuple(TypeExporter<ApiId_, detail::IndexOf<Wrappers_, Wrappers_...>::value,
                                                          typename Wrappers_::Type,
                                                          typename Wrappers_::Factory,
                                                          detail::TypeList<typename Wrappers_::Type...>,
                                                          Wrappers_>(wrappers.name())...);

        auto* temp = new ExporterSet<TypeExporter<ApiId_, detail::IndexOf<Wrappers_, Wrappers_...>::value,
                                      typename Wrappers_::Type,
                                      typename Wrappers_::Factory,
                                      detail::TypeList<typename Wrappers_::Type...>,
                                      Wrappers_>...>(std::move(exporterTuple));

        exporterSet_ = lc::detail::wrap_exporter_set(temp);
        return *temp;
    }

    void export_to(lua_State* L)
    {
        // If the API is named, push/use a new table. Otherwise, just use the global table.
        bool named = name_ && *name_;
        if (named) {
            lua_newtable(L);
            exporterSet_.export_to(L);
            lua_setglobal(L, name_);
        }
        else {
            lua_pushglobaltable(L);
            exporterSet_.export_to(L);
            lua_pop(L, 1);
        }
    }

private:
    char const* name_;
    lc::detail::ExporterSetWrapper exporterSet_;
};


template <ApiId Id_>
struct UniqueId {};

template <ApiId Id_ = 0>
auto make_api(const char* name, UniqueId<Id_> = UniqueId<Id_>{}) -> Api<Id_>
{
    return Api<Id_>(name);
}

template <ApiId Id_>
auto id() -> UniqueId<Id_>
{
    return UniqueId<Id_>{};
}

template <typename T_>
auto enum_value(char const* name, T_ value) -> lc::EnumValue<T_>
{
    return lc::EnumValue<T_>{name, value};
}

} // namespace lc

#endif // LC_HPP
