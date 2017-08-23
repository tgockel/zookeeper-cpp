#include "multi.hpp"

#include <new>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace zk
{

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
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// op                                                                                                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

op::any_data::any_data(check_data&& src) noexcept :
        check(std::move(src))
{ }

op::op(check_data&& src) noexcept :
        _type(op_type::check),
        _storage(std::move(src))
{ }

const op::check_data& op::as_check() const
{
    if (_type != op_type::check)
        throw std::logic_error("Invalid op type for as_check: " + to_string(_type));
    else
        return _storage.check;
}

// create

op::create_data::create_data(std::string path, buffer data, acl_list acl, create_mode mode) :
        path(std::move(path)),
        data(std::move(data)),
        acl(std::move(acl)),
        mode(mode)
{ }

std::ostream& operator<<(std::ostream& os, const op::create_data& self)
{
    os << '{' << self.path;
    os << ' ' << self.mode;
    os << ' ' << self.acl;
    return os << '}';
}

op op::create(std::string path, buffer data, acl_list acl, create_mode mode)
{
    return op(create_data(std::move(path), std::move(data), std::move(acl), mode));
}

op op::create(std::string path, buffer data, create_mode mode)
{
    return create(std::move(path), std::move(data), acls::open_unsafe(), mode);
}

op::any_data::any_data(create_data&& src) noexcept :
        create(std::move(src))
{ }

op::op(create_data&& src) noexcept :
        _type(op_type::create),
        _storage(std::move(src))
{ }

const op::create_data& op::as_create() const
{
    if (_type != op_type::create)
        throw std::logic_error("Invalid op type for as_create: " + to_string(_type));
    else
        return _storage.create;
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

op::any_data::any_data(erase_data&& src) noexcept :
        erase(std::move(src))
{ }

op::op(erase_data&& src) noexcept :
        _type(op_type::erase),
        _storage(std::move(src))
{ }

const op::erase_data& op::as_erase() const
{
    if (_type != op_type::erase)
        throw std::logic_error("Invalid op type for as_erase: " + to_string(_type));
    else
        return _storage.erase;
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

op::any_data::any_data(set_data&& src) noexcept :
        set(std::move(src))
{ }

op::op(set_data&& src) noexcept :
        _type(op_type::set),
        _storage(std::move(src))
{ }

const op::set_data& op::as_set() const
{
    if (_type != op_type::set)
        throw std::logic_error("Invalid op type for as_set: " + to_string(_type));
    else
        return _storage.set;
}

// generic

op::any_data::any_data(std::nullptr_t) noexcept
{ }

op::any_data::~any_data() noexcept
{
    // handled by ~op
}

template <typename T, typename... TArgs>
static void place_new(ptr<T> destination, TArgs&&... args)
        noexcept(noexcept(T(std::forward<TArgs>(args)...)))
{
    new (static_cast<ptr<void>>(destination)) T(std::forward<TArgs>(args)...);
}

op::op(const op& src) :
        _type(src._type),
        _storage(nullptr)
{
    switch (_type)
    {
    case op_type::check:  place_new(&_storage.check,  src._storage.check);  break;
    case op_type::create: place_new(&_storage.create, src._storage.create); break;
    case op_type::erase:  place_new(&_storage.erase,  src._storage.erase);  break;
    case op_type::set:    place_new(&_storage.set,    src._storage.set);    break;
    }
}

op::op(op&& src) noexcept :
        _type(src._type),
        _storage(nullptr)
{
    switch (_type)
    {
    case op_type::check:  place_new(&_storage.check,  std::move(src._storage.check));  break;
    case op_type::create: place_new(&_storage.create, std::move(src._storage.create)); break;
    case op_type::erase:  place_new(&_storage.erase,  std::move(src._storage.erase));  break;
    case op_type::set:    place_new(&_storage.set,    std::move(src._storage.set));    break;
    }
}

op::~op() noexcept
{
    switch (_type)
    {
    case op_type::check:  _storage.check.~auto();  break;
    case op_type::create: _storage.create.~auto(); break;
    case op_type::erase:  _storage.erase.~auto();  break;
    case op_type::set:    _storage.set.~auto();    break;
    }
}

std::ostream& operator<<(std::ostream& os, const op& self)
{
    os << self.type();
    switch (self.type())
    {
    case op_type::check:  return os << self.as_check();
    case op_type::create: return os << self.as_create();
    case op_type::erase:  return os << self.as_erase();
    case op_type::set:    return os << self.as_set();
    default:              return os << "{ ??? }";
    }
}

std::string to_string(const op& self)
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

}
