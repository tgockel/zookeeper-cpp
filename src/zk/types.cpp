#include "types.hpp"

#include <ostream>
#include <sstream>
#include <utility>

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
// version                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const version& self)
{
    os << "version(";
    if (self == version::any())
        os << "any";
    else if (self == version::invalid())
        os << "invalid";
    else
        os << self.value;
    return os << ')';
}

std::string to_string(const version& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acl_version                                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const acl_version& self)
{
    os << "acl_version(";
    if (self == acl_version::any())
        os << "any";
    else if (self == acl_version::invalid())
        os << "invalid";
    else
        os << self.value;
    return os << ')';
}

std::string to_string(const acl_version& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// child_version                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const child_version& self)
{
    os << "child_version(";
    if (self == child_version::any())
        os << "any";
    else if (self == child_version::invalid())
        os << "invalid";
    else
        os << self.value;
    return os << ')';
}

std::string to_string(const child_version& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// transaction_id                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const transaction_id& self)
{
    return os << "transaction_id(" << self.value << ')';
}

std::string to_string(const transaction_id& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// stat                                                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const stat& self)
{
    os << "{data_version="   << self.data_version.value;
    os << " child_version="  << self.child_version.value;
    os << " acl_version="    << self.acl_version.value;
    os << " data_size="      << self.data_size;
    os << " children_count=" << self.children_count;
    os << " ephemeral="      << (self.is_ephemeral() ? "true" : "false");
    return os << '}';
}

std::string to_string(const stat& self)
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
    case state::connected:             return os << "connected";
    case state::read_only:             return os << "read_only";
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create_mode                                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const create_mode& mode)
{
    if (mode == create_mode::normal)
        return os << "normal";

    bool first = true;
    auto tick = [&] { return std::exchange(first, false) ? "" : "|"; };
    if (is_set(mode, create_mode::ephemeral))  os << tick() << "ephemeral";
    if (is_set(mode, create_mode::sequential)) os << tick() << "sequential";
    if (is_set(mode, create_mode::container))  os << tick() << "container";

    return os;
}

std::string to_string(const create_mode& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

}
