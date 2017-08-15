#include "watch.hpp"

#include <ostream>
#include <sstream>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// event_type                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const event_type& self)
{
    switch (self)
    {
    case event_type::error:        return os << "error";
    case event_type::created:      return os << "created";
    case event_type::erased:       return os << "erased";
    case event_type::changed:      return os << "changed";
    case event_type::child:        return os << "child";
    case event_type::session:      return os << "session";
    case event_type::not_watching: return os << "not_watching";
    default:                       return os << "event_type(" << static_cast<int>(self) << ')';
    };
}

std::string to_string(const event_type& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state                                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const state& self)
{
    switch (self)
    {
    case state::closed:                return os << "closed";
    case state::connecting:            return os << "connecting";
    case state::associating:           return os << "associating";
    case state::connected:             return os << "connected";
    case state::read_only:             return os << "read_only";
    case state::not_connected:         return os << "not_connected";
    case state::expired_session:       return os << "expired_session";
    case state::authentication_failed: return os << "authentication_failed";
    default:                           return os << "state(" << static_cast<int>(self) << ')';
    }
}

std::string to_string(const state& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

}
