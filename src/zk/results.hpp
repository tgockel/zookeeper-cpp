/// \file
/// Describes the various result types of \c client operations.
#pragma once

#include <zk/config.hpp>

#include <iosfwd>
#include <string>
#include <vector>

#include "acl.hpp"
#include "buffer.hpp"
#include "future.hpp"
#include "optional.hpp"
#include "types.hpp"

namespace zk
{

/// \addtogroup Client
/// \{

/// The result type of \c client::get.
class get_result final
{
public:
    explicit get_result(buffer data, const zk::stat& stat) noexcept;

    get_result(const get_result&)            = default;
    get_result& operator=(const get_result&) = default;

    get_result(get_result&&)            = default;
    get_result& operator=(get_result&&) = default;

    ~get_result() noexcept;

    /// \{
    /// The data read from the entry.
    const buffer& data() const & { return _data; }
    buffer&       data() &       { return _data; }
    buffer        data() &&      { return std::move(_data); }
    /// \}

    /// \{
    /// The \ref zk::stat of the entry at the time it was read. The most useful value of the returned value is
    /// \ref stat::data_version.
    const zk::stat& stat() const { return _stat; }
    zk::stat&       stat()       { return _stat; }
    /// \}

private:
    buffer   _data;
    zk::stat _stat;
};

std::ostream& operator<<(std::ostream&, const get_result&);

std::string to_string(const get_result&);

/// The result type of \c client::get_children.
class get_children_result final
{
public:
    using children_list_type = std::vector<std::string>;

public:
    explicit get_children_result(children_list_type children, const stat& parent_stat) noexcept;

    get_children_result(const get_children_result&)            = default;
    get_children_result& operator=(const get_children_result&) = default;

    get_children_result(get_children_result&&)            = default;
    get_children_result& operator=(get_children_result&&) = default;

    ~get_children_result() noexcept;

    /// \{
    /// The list of children of the originally-queried node. Note that there is no guarantee on ordering of this list.
    const children_list_type& children() const & { return _children; }
    children_list_type&       children() &       { return _children; }
    children_list_type        children() &&      { return std::move(_children); }
    /// \}

    /// \{
    /// The \ref zk::stat of the entry queried (the parent of the \ref children).
    const stat& parent_stat() const { return _parent_stat; }
    stat&       parent_stat()       { return _parent_stat; }
    /// \}

private:
    children_list_type _children;
    stat               _parent_stat;
};

std::ostream& operator<<(std::ostream&, const get_children_result&);

std::string to_string(const get_children_result&);

/// The result type of \ref client::exists.
class exists_result final
{
public:
    explicit exists_result(const optional<zk::stat>& stat) noexcept;

    exists_result(const exists_result&)            = default;
    exists_result& operator=(const exists_result&) = default;

    exists_result(exists_result&&)            = default;
    exists_result& operator=(exists_result&&) = default;

    ~exists_result() noexcept;

    /// \{
    /// An \c exists_result is \c true if the entry exists.
    explicit operator bool() const { return bool(_stat); }
    bool operator!() const         { return !_stat; }
    /// \}

    /// \{
    /// The \ref zk::stat of the entry if it exists. If it does not exist, this value will be \c nullopt.
    const optional<zk::stat>& stat() const { return _stat; }
    optional<zk::stat>&       stat()       { return _stat; }
    /// \}

private:
    optional<zk::stat> _stat;
};

std::ostream& operator<<(std::ostream&, const exists_result&);

std::string to_string(const exists_result&);

/// The result type of \ref client::create.
class create_result final
{
public:
    explicit create_result(std::string name) noexcept;

    create_result(const create_result&)            = default;
    create_result& operator=(const create_result&) = default;

    create_result(create_result&&)            = default;
    create_result& operator=(create_result&&) = default;

    ~create_result() noexcept;

    /// \{
    /// The name of the created entry. How useful this is depends on the value of \ref create_mode passed to the create
    /// operation. If \ref create_mode::sequential was set, this value must be used to see what was created. In all
    /// other cases, the name is the same as the path which was passed in.
    const std::string& name() const & { return _name; }
    std::string&       name() &       { return _name; }
    std::string        name() &&      { return std::move(_name); }
    /// \}

private:
    std::string _name;
};

std::ostream& operator<<(std::ostream&, const create_result&);

std::string to_string(const create_result&);

/// The result type of \ref client::set.
class set_result final
{
public:
    explicit set_result(const zk::stat& stat) noexcept;

    set_result(const set_result&)            = default;
    set_result& operator=(const set_result&) = default;

    set_result(set_result&&)            = default;
    set_result& operator=(set_result&&) = default;

    ~set_result() noexcept;

    /// \{
    /// The \ref zk::stat of the entry after the set operation.
    const zk::stat& stat() const { return _stat; }
    zk::stat&       stat()       { return _stat; }
    /// \}

private:
    zk::stat _stat;
};

std::ostream& operator<<(std::ostream&, const set_result&);

std::string to_string(const set_result&);

/// The result type of \ref client::get_acl.
class get_acl_result final
{
public:
    explicit get_acl_result(zk::acl acl, const zk::stat& stat) noexcept;

    get_acl_result(const get_acl_result&)            = default;
    get_acl_result& operator=(const get_acl_result&) = default;

    get_acl_result(get_acl_result&&)            = default;
    get_acl_result& operator=(get_acl_result&&) = default;

    ~get_acl_result() noexcept;

    /// \{
    /// The \ref zk::acl of the entry.
    const zk::acl& acl() const & { return _acl; }
    zk::acl&       acl() &       { return _acl; }
    zk::acl        acl() &&      { return std::move(_acl); }
    /// \}

    /// \{
    /// The \ref zk::stat of the entry at the time it was read. The most useful value of the returned value is
    /// \ref stat::acl_version.
    const zk::stat& stat() const { return _stat; }
    zk::stat&       stat()       { return _stat; }
    /// \}

private:
    zk::acl  _acl;
    zk::stat _stat;
};

std::ostream& operator<<(std::ostream&, const get_acl_result&);

std::string to_string(const get_acl_result&);

/// Data delivered when a watched event triggers.
///
/// \note
/// If you are familiar with the ZooKeeper C API, the limited information delivered might seem very simplistic. Since
/// this API only supports non-global watches, the extra parameters are not helpful and generally unsafe. As an example,
/// the \c path parameter is not included. It is not helpful to include, since you already know the path you specified
/// when you set the watch in the first place. Furthermore, it is unsafe, as the contents addressed by the pointer are
/// only safe in the callback thread. While we could copy the path into an \c std::string, this would require an
/// allocation on every delivery, which is very intrusive.
class event final
{
public:
    explicit event(event_type type, zk::state state) noexcept;

    event(const event&)            = default;
    event& operator=(const event&) = default;

    event(event&&)            = default;
    event& operator=(event&&) = default;

    /// The type of event that occurred.
    const event_type& type() const { return _type; }

    /// The state of the connection when the event occurred. Keep in mind that it might be different when the value is
    /// delivered.
    const zk::state& state() const { return _state; }

private:
    event_type _type;
    zk::state  _state;
};

std::ostream& operator<<(std::ostream&, const event&);

std::string to_string(const event&);

/// The result type of \ref client::watch.
class watch_result final
{
public:
    explicit watch_result(get_result initial, future<event> next) noexcept;

    watch_result(const watch_result&)            = delete;
    watch_result& operator=(const watch_result&) = delete;

    watch_result(watch_result&&)            = default;
    watch_result& operator=(watch_result&&) = default;

    ~watch_result() noexcept;

    /// \{
    /// The initial result of the fetch.
    const get_result& initial() const & { return _initial; }
    get_result&       initial() &       { return _initial; }
    get_result        initial() &&      { return std::move(_initial); }
    /// \}

    /// \{
    /// Future to be delivered when the watch is triggered.
    const future<event>& next() const & { return _next; }
    future<event>&       next() &       { return _next; }
    future<event>        next() &&      { return std::move(_next); }
    /// \}

private:
    get_result    _initial;
    future<event> _next;
};

std::ostream& operator<<(std::ostream&, const watch_result&);

std::string to_string(const watch_result&);

/// The result type of \ref client::watch_children.
class watch_children_result final
{
public:
    explicit watch_children_result(get_children_result initial, future<event> next) noexcept;

    watch_children_result(const watch_children_result&)            = delete;
    watch_children_result& operator=(const watch_children_result&) = delete;

    watch_children_result(watch_children_result&&)            = default;
    watch_children_result& operator=(watch_children_result&&) = default;

    ~watch_children_result() noexcept;

    /// \{
    /// The initial result of the fetch.
    const get_children_result& initial() const & { return _initial; }
    get_children_result&       initial() &       { return _initial; }
    get_children_result        initial() &&      { return std::move(_initial); }
    /// \}

    /// \{
    /// Future to be delivered when the watch is triggered.
    const future<event>& next() const & { return _next; }
    future<event>&       next() &       { return _next; }
    future<event>        next() &&      { return std::move(_next); }
    /// \}

private:
    get_children_result _initial;
    future<event>       _next;
};

std::ostream& operator<<(std::ostream&, const watch_children_result&);

std::string to_string(const watch_children_result&);

/// The result type of \ref client::watch_exists.
class watch_exists_result final
{
public:
    explicit watch_exists_result(exists_result initial, future<event> next) noexcept;

    watch_exists_result(const watch_exists_result&)            = delete;
    watch_exists_result& operator=(const watch_exists_result&) = delete;

    watch_exists_result(watch_exists_result&&)            = default;
    watch_exists_result& operator=(watch_exists_result&&) = default;

    ~watch_exists_result() noexcept;

    /// \{
    /// The initial result of the fetch.
    const exists_result& initial() const & { return _initial; }
    exists_result&       initial() &       { return _initial; }
    exists_result        initial() &&      { return std::move(_initial); }
    /// \}

    /// \{
    /// Future to be delivered when the watch is triggered.
    const future<event>& next() const & { return _next; }
    future<event>&       next() &       { return _next; }
    future<event>        next() &&      { return std::move(_next); }
    /// \}

private:
    exists_result _initial;
    future<event> _next;
};

std::ostream& operator<<(std::ostream&, const watch_exists_result&);

std::string to_string(const watch_exists_result&);

/// \}

}
