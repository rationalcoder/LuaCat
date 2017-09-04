#ifndef LG_STORAGE_HPP
#define LG_STORAGE_HPP
#include <lg/detail/lg_common.hpp>
#include <lg/detail/lg_utility.hpp>

namespace lg
{
namespace detail
{

class GlobalStorage
{
public:
    static GlobalStorage& instance()
    {
        static GlobalStorage storage;
        return storage;
    }

    GlobalStorage(const GlobalStorage&) = delete;
    GlobalStorage(GlobalStorage&&) = delete;
    GlobalStorage& operator=(const GlobalStorage&) = delete;

public:
    void add_api_entry(ApiId id, uint16_t numTypes, uint16_t numMetatableTypes)
    {
        std::size_t apiIndex = id+1;
        apiNamesBuffer_.reserve_initialized(apiIndex);
        apiNamesBuffer_[i].reserve_initialized(numTypes);

        apiTypeMetatablesBuffer_.reserve_initialized(numMetatableTypes, LUA_REFNIL);
        apiTypeMetatablesBuffer_[apiIndex];
    }

    char const** api_names_at(ApiId id)
    {
        return apiNamesBuffer_[id].data();
    }

    lua_Integer* api_metatables_at(ApiId id)
    {
        return apiTypeMetatablesBuffer_[id].data();
    }

private:
    GlobalStorage() = default;

private:
    lg::detail::DynamicArray<lg::detail::DynamicArray<char const*>> apiNamesBuffer_;
    lg::detail::DynamicArray<lg::detail::DynamicArray<lua_Integer>> apiTypeMetatablesBuffer_;
};

} // namespace detail
} // namespace lg

#endif // LG_STORAGE_HPP
