#include "acl.hpp"

#include <ostream>
#include <sstream>
#include <tuple>
#include <utility>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// permission                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const permission& self)
{
    if (self == permission::none)
        return os << "none";
    else if (self == permission::all)
        return os << "all";

    bool first = true;
    auto tick = [&] { return std::exchange(first, false) ? "" : "|"; };
    if (allows(self, permission::read))   os << tick() << "read";
    if (allows(self, permission::write))  os << tick() << "write";
    if (allows(self, permission::create)) os << tick() << "create";
    if (allows(self, permission::erase))  os << tick() << "erase";
    if (allows(self, permission::admin))  os << tick() << "admin";

    return os;
}

std::string to_string(const permission& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acl                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

acl::acl(std::string scheme, std::string id, permission permissions) :
        _scheme(std::move(scheme)),
        _id(std::move(id)),
        _permissions(permissions)
{ }

acl::~acl() noexcept
{ }

std::size_t hash(const acl& self)
{
    return std::hash<std::string>{}(self.scheme())
         ^ std::hash<std::string>{}(self.id())
         ^ std::hash<unsigned int>{}(static_cast<unsigned int>(self.permissions()));
}

bool operator==(const acl& lhs, const acl& rhs)
{
    return lhs.scheme()      == rhs.scheme()
        && lhs.id()          == rhs.id()
        && lhs.permissions() == rhs.permissions();
}

bool operator!=(const acl& lhs, const acl& rhs)
{
    return !(lhs == rhs);
}

bool operator< (const acl& lhs, const acl& rhs)
{
    return std::tie(lhs.scheme(), lhs.id(), lhs.permissions()) < std::tie(rhs.scheme(), rhs.id(), rhs.permissions());
}

bool operator<=(const acl& lhs, const acl& rhs)
{
    return !(rhs < lhs);
}

bool operator> (const acl& lhs, const acl& rhs)
{
    return rhs < lhs;
}

bool operator>=(const acl& lhs, const acl& rhs)
{
    return !(lhs < rhs);
}

std::ostream& operator<<(std::ostream& os, const acl& self)
{
    os << '(' << self.scheme();
    if (!self.id().empty())
        os << ':' << self.id();
    os << ", " << self.permissions() << ')';
    return os;
}

std::string to_string(const acl& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acl_list                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

acl_list::acl_list(std::vector<acl> acls) noexcept :
        _impl(std::move(acls))
{ }

acl_list::~acl_list() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const acl_list& self)
{
    os << '{';
    bool first = true;
    for (const auto& x : self)
    {
        if (first)
            first = false;
        else
            os << ", ";
        os << x;
    }
    return os << '}';
}

std::string to_string(const acl_list& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acls                                                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const acl_list& acls::creator_all()
{
    static acl_list instance = { { "auth", "", permission::all } };
    return instance;
}

const acl_list& acls::open_unsafe()
{
    static acl_list instance = { { "world", "anyone", permission::all } };
    return instance;
}

const acl_list& acls::read_unsafe()
{
    static acl_list instance = { { "world", "anyone", permission::read } };
    return instance;
}

}
