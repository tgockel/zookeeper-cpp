#include "multi.hpp"

#include <new>
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
// multi_op                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

multi_op::multi_op(std::vector<op> ops) noexcept :
        _ops(std::move(ops))
{ }

multi_op::~multi_op() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const multi_op& self)
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// multi_result                                                                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

multi_result::part::part(op_type type, std::nullptr_t) noexcept :
        _type(type),
        _storage(std::monostate())
{ }

multi_result::part::part(create_result res) noexcept :
        _type(op_type::create),
        _storage(std::move(res))
{ }

multi_result::part::part(set_result res) noexcept :
        _type(op_type::set),
        _storage(std::move(res))
{ }

multi_result::part::part(const part& src) = default;

multi_result::part::part(part&& src) noexcept :
        _type(src._type),
        _storage(std::move(src._storage))
{ }

multi_result::part::~part() noexcept = default;

template <typename T>
const T& multi_result::part::as(ptr<const char> operation) const
{
    try
    {
        return std::get<T>(_storage);
    }
    catch (const std::bad_variant_access&)
    {
        throw std::logic_error( std::string("Invalid op type for multi_result::")
                              + std::string(operation)
                              + std::string(": ")
                              + to_string(type())
                              );
    }
}

const create_result& multi_result::part::as_create() const
{
    return as<create_result>("as_create");
}

const set_result& multi_result::part::as_set() const
{
    return as<set_result>("as_set");
}

multi_result::multi_result(std::vector<part> parts) noexcept :
        _parts(std::move(parts))
{ }

multi_result::~multi_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const multi_result::part& self)
{
    switch (self.type())
    {
        case op_type::create: return os << self.as_create();
        case op_type::set:    return os << self.as_set();
        default:              return os << self.type() << "_result{}";
    }
}

std::ostream& operator<<(std::ostream& os, const multi_result& self)
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

std::string to_string(const multi_result::part& self)
{
    return to_string_generic(self);
}

std::string to_string(const multi_result& self)
{
    return to_string_generic(self);
}

}
