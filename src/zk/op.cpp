#include "op.hpp"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace zk
{

template <typename T>
static std::string to_string_generic(const T& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// op_type                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const op_type& self)
{
    switch (self)
    {
    case op_type::check:  return os << "check";
    case op_type::create: return os << "create";
    case op_type::erase:  return os << "erase";
    case op_type::set:    return os << "set";
    default:              return os << "op_type(" << static_cast<int>(self) << ')';
    }
}

std::string to_string(const op_type& self)
{
    return to_string_generic(self);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// op                                                                                                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

op::op(any_data&& src) noexcept :
        _storage(std::move(src))
{ }

op::op(const op& src) = default;

op::op(op&& src) noexcept :
        _storage(std::move(src._storage))
{ }

op::~op() noexcept = default;

op_type op::type() const
{
    return std::visit([] (const auto& x) { return x.type(); }, _storage);
}

template <typename T>
const T& op::as(ptr<const char> operation) const
{
    try
    {
        return std::get<T>(_storage);
    }
    catch (const std::bad_variant_access&)
    {
        throw std::logic_error( std::string("Invalid op type for op::")
                              + std::string(operation)
                              + std::string(": ")
                              + to_string(type())
                              );
    }
}

// check

op::check_data::check_data(std::string path, version check_) :
        path(std::move(path)),
        check(check_)
{ }

std::ostream& operator<<(std::ostream& os, const op::check_data& self)
{
    os << '{' << self.path;
    os << ' ' << self.check;
    return os << '}';
}

op op::check(std::string path, version check_)
{
    return op(check_data(std::move(path), check_));
}

const op::check_data& op::as_check() const
{
    return as<check_data>("as_check");
}

// create

op::create_data::create_data(std::string path, buffer data, acl rules, create_mode mode) :
        path(std::move(path)),
        data(std::move(data)),
        rules(std::move(rules)),
        mode(mode)
{ }

std::ostream& operator<<(std::ostream& os, const op::create_data& self)
{
    os << '{' << self.path;
    os << ' ' << self.mode;
    os << ' ' << self.rules;
    return os << '}';
}

op op::create(std::string path, buffer data, acl rules, create_mode mode)
{
    return op(create_data(std::move(path), std::move(data), std::move(rules), mode));
}

op op::create(std::string path, buffer data, create_mode mode)
{
    return create(std::move(path), std::move(data), acls::open_unsafe(), mode);
}

const op::create_data& op::as_create() const
{
    return as<create_data>("as_create");
}

// erase

op::erase_data::erase_data(std::string path, version check) :
        path(std::move(path)),
        check(check)
{ }

std::ostream& operator<<(std::ostream& os, const op::erase_data& self)
{
    os << '{' << self.path;
    os << ' ' << self.check;
    return os << '}';
}

op op::erase(std::string path, version check)
{
    return op(erase_data(std::move(path), check));
}

const op::erase_data& op::as_erase() const
{
    return as<erase_data>("as_erase");
}

// set

op::set_data::set_data(std::string path, buffer data, version check) :
        path(std::move(path)),
        data(std::move(data)),
        check(check)
{ }

std::ostream& operator<<(std::ostream& os, const op::set_data& self)
{
    os << '{' << self.path;
    os << ' ' << self.check;
    return os << '}';
}

op op::set(std::string path, buffer data, version check)
{
    return op(set_data(std::move(path), std::move(data), check));
}

const op::set_data& op::as_set() const
{
    return as<set_data>("as_set");
}

// generic

std::ostream& operator<<(std::ostream& os, const op& self)
{
    os << self.type();
    std::visit([&] (const auto& x) { os << x; }, self._storage);
    return os;
}

std::string to_string(const op& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

}
