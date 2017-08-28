#ifndef LG_STORAGE_HPP
#define LG_STORAGE_HPP
#include <lg/detail/lg_common.hpp>

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

private:
    GlobalStorage() {}
};

} // namespace detail
} // namespace lg

#endif // LG_STORAGE_HPP
