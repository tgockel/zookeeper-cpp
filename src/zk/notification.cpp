#include "notification.hpp"

#include <ostream>
#include <type_traits>

namespace zk
{

void notification::reset()
{
    _data = std::monostate;
    _tracker.reset();
}

std::ostream& operator<<(std::ostream& os, const notification& self)
{
    std::visit([&] (const auto& x)
               {
                   if constexpr (std::is_same_v<decltype(x), std::monostate_t>)
                       os << "()";
                   else
                       os << x;
               }, self._data);
    return os;
}

}
