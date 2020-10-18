#include "results.hpp"

#include <ostream>
#include <sstream>
#include <utility>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities                                                                                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// These tags are used during print_buffer overload resolution. If TBuffer type is not
// applicable (ill-formed) for one of the overloads, then SFINAE selects the appropriate overload.
// But there are cases (e.g. when TBuffer is std::string) when both of the print_buffer
// are well-formed and therefore the call to print_buffer would be ambiguous. These tags
// allows to select one of the overloads by using more derived tag (print_buffer_content_tag).
struct print_buffer_length_tag {};
struct print_buffer_content_tag  : public print_buffer_length_tag {};

template <typename TBuffer>
auto print_buffer(std::ostream& os, const TBuffer& buf, struct print_buffer_content_tag)
        -> decltype((os << buf), void())
{
    os << buf;
}

template <typename TBuffer>
void print_buffer(std::ostream& os, const TBuffer& buf, struct print_buffer_length_tag)
{
    os << "size=" << buf.size();
}

template <typename TRange>
void print_range(std::ostream& os, const TRange& range)
{
    os << '[';
    bool first = true;
    for (const auto& x : range)
    {
        if (first)
            first = false;
        else
            os << ", ";
        os << x;
    }
    os << ']';
}

template <typename T>
std::string to_string_generic(const T& x)
{
    std::ostringstream os;
    os << x;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_result                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

get_result::get_result(buffer data, const zk::stat& stat) noexcept :
        _data(std::move(data)),
        _stat(stat)
{ }

get_result::~get_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const get_result& self)
{
    os << "get_result{";
    print_buffer(os, self.data(), print_buffer_content_tag {});
    os << ' ' << self.stat();
    return os << '}';
}

std::string to_string(const get_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<get_result>);
static_assert(std::is_copy_assignable_v<get_result>);
static_assert(std::is_nothrow_move_constructible_v<get_result>);
static_assert(std::is_nothrow_move_assignable_v<get_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_children_result                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

get_children_result::get_children_result(children_list_type children, const stat& parent_stat) noexcept :
        _children(std::move(children)),
        _parent_stat(parent_stat)
{ }

get_children_result::~get_children_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const get_children_result& self)
{
    os << "get_children_result{";
    print_range(os, self.children());
    os << " parent=" << self.parent_stat();
    return os << '}';
}

std::string to_string(const get_children_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<get_children_result>);
static_assert(std::is_copy_assignable_v<get_children_result>);
static_assert(std::is_nothrow_move_constructible_v<get_children_result>);
static_assert(std::is_nothrow_move_assignable_v<get_children_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// exists_result                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

exists_result::exists_result(const optional<zk::stat>& stat) noexcept :
        _stat(stat)
{ }

exists_result::~exists_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const exists_result& self)
{
    os << "exists_result{";
    if (self)
        os << *self.stat();
    else
        os << "(no)";
    return os << '}';
}

std::string to_string(const exists_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<exists_result>);
static_assert(std::is_copy_assignable_v<exists_result>);
static_assert(std::is_nothrow_move_constructible_v<exists_result>);
static_assert(std::is_nothrow_move_assignable_v<exists_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create_result                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

create_result::create_result(std::string name) noexcept :
        _name(std::move(name))
{ }

create_result::~create_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const create_result& self)
{
    return os << "create_result{name=" << self.name() << '}';
}

std::string to_string(const create_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<create_result>);
static_assert(std::is_copy_assignable_v<create_result>);
static_assert(std::is_nothrow_move_constructible_v<create_result>);
static_assert(std::is_nothrow_move_assignable_v<create_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set_result                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

set_result::set_result(const zk::stat& stat) noexcept :
        _stat(stat)
{ }

set_result::~set_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const set_result& self)
{
    return os << "set_result{" << self.stat() << '}';
}

std::string to_string(const set_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<set_result>);
static_assert(std::is_copy_assignable_v<set_result>);
static_assert(std::is_nothrow_move_constructible_v<set_result>);
static_assert(std::is_nothrow_move_assignable_v<set_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_acl_result                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

get_acl_result::get_acl_result(zk::acl acl, const zk::stat& stat) noexcept :
        _acl(std::move(acl)),
        _stat(stat)
{ }

get_acl_result::~get_acl_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const get_acl_result& self)
{
    return os << "get_acl_result{" << self.acl() << ' ' << self.stat() << '}';
}

std::string to_string(const get_acl_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<get_acl_result>);
static_assert(std::is_copy_assignable_v<get_acl_result>);
static_assert(std::is_nothrow_move_constructible_v<get_acl_result>);
static_assert(std::is_nothrow_move_assignable_v<get_acl_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// event                                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

event::event(event_type type, zk::state state) noexcept :
        _type(type),
        _state(state)
{ }

std::ostream& operator<<(std::ostream& os, const event& self)
{
    return os << "event{" << self.type() << " | " << self.state() << '}';
}

std::string to_string(const event& self)
{
    return to_string_generic(self);
}

static_assert(std::is_copy_constructible_v<event>);
static_assert(std::is_copy_assignable_v<event>);
static_assert(std::is_nothrow_move_constructible_v<event>);
static_assert(std::is_nothrow_move_assignable_v<event>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// watch_result                                                                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

watch_result::watch_result(get_result initial, future<event> next) noexcept :
        _initial(std::move(initial)),
        _next(std::move(next))
{ }

watch_result::~watch_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const watch_result& self)
{
    return os << "watch_result{initial=" << self.initial() << '}';
}

std::string to_string(const watch_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_nothrow_move_constructible_v<watch_result>);
static_assert(std::is_nothrow_move_assignable_v<watch_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// watch_children_result                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

watch_children_result::watch_children_result(get_children_result initial, future<event> next) noexcept :
        _initial(std::move(initial)),
        _next(std::move(next))
{ }

watch_children_result::~watch_children_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const watch_children_result& self)
{
    return os << "watch_children_result{initial=" << self.initial() << '}';
}

std::string to_string(const watch_children_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_nothrow_move_constructible_v<watch_children_result>);
static_assert(std::is_nothrow_move_assignable_v<watch_children_result>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// watch_exists_result                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

watch_exists_result::watch_exists_result(exists_result initial, future<event> next) noexcept :
        _initial(std::move(initial)),
        _next(std::move(next))
{ }

watch_exists_result::~watch_exists_result() noexcept
{ }

std::ostream& operator<<(std::ostream& os, const watch_exists_result& self)
{
    return os << "watch_exists_result{initial=" << self.initial() << '}';
}

std::string to_string(const watch_exists_result& self)
{
    return to_string_generic(self);
}

static_assert(std::is_nothrow_move_constructible_v<watch_exists_result>);
static_assert(std::is_nothrow_move_assignable_v<watch_exists_result>);

}
