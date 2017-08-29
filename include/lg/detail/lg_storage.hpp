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
    void add_entry(ApiId id, uint32_t numTypes)
    {
        expand_if_necessary(id, numTypes);
    }

    const char* const* api_names_at(ApiId) {}
    TypeId api_metatables_at(ApiId) {}

private:
    GlobalStorage() = default;

    LG_FORCE_INLINE void expand_if_necessary(ApiId, uint32_t) {}

private:
};

} // namespace detail
} // namespace lg

#endif // LG_STORAGE_HPP
