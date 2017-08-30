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
// acl_rule                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

acl_rule::acl_rule(std::string scheme, std::string id, permission permissions) :
        _scheme(std::move(scheme)),
        _id(std::move(id)),
        _permissions(permissions)
{ }

acl_rule::~acl_rule() noexcept
{ }

std::size_t hash(const acl_rule& self)
{
    return std::hash<std::string>{}(self.scheme())
         ^ std::hash<std::string>{}(self.id())
         ^ std::hash<unsigned int>{}(static_cast<unsigned int>(self.permissions()));
}

bool operator==(const acl_rule& lhs, const acl_rule& rhs)
{
    return lhs.scheme()      == rhs.scheme()
        && lhs.id()          == rhs.id()
        && lhs.permissions() == rhs.permissions();
}

bool operator!=(const acl_rule& lhs, const acl_rule& rhs)
{
    return !(lhs == rhs);
}

bool operator< (const acl_rule& lhs, const acl_rule& rhs)
{
    return std::tie(lhs.scheme(), lhs.id(), lhs.permissions()) < std::tie(rhs.scheme(), rhs.id(), rhs.permissions());
}

bool operator<=(const acl_rule& lhs, const acl_rule& rhs)
{
    return !(rhs < lhs);
}

bool operator> (const acl_rule& lhs, const acl_rule& rhs)
{
    return rhs < lhs;
}

bool operator>=(const acl_rule& lhs, const acl_rule& rhs)
{
    return !(lhs < rhs);
}

std::ostream& operator<<(std::ostream& os, const acl_rule& self)
{
    os << '(' << self.scheme();
    if (!self.id().empty())
        os << ':' << self.id();
    os << ", " << self.permissions() << ')';
    return os;
}

std::string to_string(const acl_rule& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acl                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

acl::acl(std::vector<acl_rule> rules) noexcept :
        _impl(std::move(rules))
{ }

acl::~acl() noexcept
{ }

bool operator==(const acl& lhs, const acl& rhs)
{
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

bool operator!=(const acl& lhs, const acl& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const acl& self)
{
    os << '[';
    bool first = true;
    for (const auto& x : self)
    {
        if (first)
            first = false;
        else
            os << ", ";
        os << x;
    }
    return os << ']';
}

std::string to_string(const acl& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acls                                                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const acl& acls::creator_all()
{
    static acl instance = { { "auth", "", permission::all } };
    return instance;
}

const acl& acls::open_unsafe()
{
    static acl instance = { { "world", "anyone", permission::all } };
    return instance;
}

const acl& acls::read_unsafe()
{
    static acl instance = { { "world", "anyone", permission::read } };
    return instance;
}

}
