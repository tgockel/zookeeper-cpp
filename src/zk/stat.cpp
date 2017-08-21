#include "stat.hpp"

#include <ostream>
#include <sstream>

namespace zk
{

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

}
