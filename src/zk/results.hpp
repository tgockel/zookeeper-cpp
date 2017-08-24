/** \file
 *  Describes the various result types of \c client operations.
**/
#pragma once

#include <zk/config.hpp>

#include <iosfwd>
#include <string>
#include <vector>

#include "buffer.hpp"
#include "future.hpp"
#include "optional.hpp"
#include "types.hpp"

namespace zk
{

/** The result type of \c client::get. **/
class get_result final
{
public:
    explicit get_result(buffer data, const zk::stat& stat) noexcept;

    virtual ~get_result() noexcept;

    const buffer& data() const & { return _data; }
    buffer&       data() &       { return _data; }
    buffer        data() &&      { return std::move(_data); }

    const zk::stat& stat() const { return _stat; }
    zk::stat&       stat()       { return _stat; }

private:
    buffer   _data;
    zk::stat _stat;
};

std::ostream& operator<<(std::ostream&, const get_result&);

std::string to_string(const get_result&);

/** The result type of \c client::get_children. **/
class get_children_result final
{
public:
    using children_list_type = std::vector<std::string>;

public:
    explicit get_children_result(children_list_type children, const stat& parent_stat) noexcept;

    ~get_children_result() noexcept;

    const children_list_type& children() const & { return _children; }
    children_list_type&       children() &       { return _children; }
    children_list_type        children() &&      { return std::move(_children); }

    const stat& parent_stat() const { return _parent_stat; }
    stat&       parent_stat()       { return _parent_stat; }

private:
    children_list_type _children;
    stat               _parent_stat;
};

std::ostream& operator<<(std::ostream&, const get_children_result&);

std::string to_string(const get_children_result&);

/** The result type of \c client::exists. **/
class exists_result final
{
public:
    explicit exists_result(const optional<zk::stat>& stat) noexcept;

    ~exists_result() noexcept;

    explicit operator bool() const { return bool(_stat); }
    bool operator!() const         { return !_stat; }

    const optional<zk::stat>& stat() const { return _stat; }
    optional<zk::stat>&       stat()       { return _stat; }

private:
    optional<zk::stat> _stat;
};

std::ostream& operator<<(std::ostream&, const exists_result&);

std::string to_string(const exists_result&);

/** The result type of \c client::create. **/
class create_result final
{
public:
    explicit create_result(std::string name) noexcept;

    ~create_result() noexcept;

    const std::string& name() const & { return _name; }
    std::string&       name() &       { return _name; }
    std::string        name() &&      { return std::move(_name); }

private:
    std::string _name;
};

std::ostream& operator<<(std::ostream&, const create_result&);

std::string to_string(const create_result&);

/** The result type of \c client::set. **/
class set_result final
{
public:
    explicit set_result(const zk::stat& stat) noexcept;

    ~set_result() noexcept;

    const zk::stat& stat() const { return _stat; }
    zk::stat&       stat()       { return _stat; }

private:
    zk::stat _stat;
};

std::ostream& operator<<(std::ostream&, const set_result&);

std::string to_string(const set_result&);

/** Data delivered when a watched event triggers.
 *
 *  \note
 *  If you are familiar with the ZooKeeper C API, the limited information delivered might seem very simplistic. Since
 *  this API only supports non-global watches, the extra parameters are not helpful and generally unsafe. As an example,
 *  the \c path parameter is not included. It is not helpful to include, since you already know the path you specified
 *  when you set the watch in the first place. Furthermore, it is unsafe, as the contents addressed by the pointer are
 *  only safe in the callback thread. While we could copy the path into an \c std::string, this would require an
 *  allocation on every delivery, which is very intrusive.
**/
class event final
{
public:
    explicit event(event_type type, zk::state state) noexcept;

    /** The type of event that occurred. **/
    const event_type& type() const { return _type; }

    /** The state of the connection when the event occurred. Keep in mind that it might be different when the value is
     *  delivered.
    **/
    const zk::state& state() const { return _state; }

private:
    event_type _type;
    zk::state  _state;
};

std::ostream& operator<<(std::ostream&, const event&);

std::string to_string(const event&);

/** The result type of \c client::watch. **/
class watch_result final
{
public:
    explicit watch_result(get_result initial, future<event> next) noexcept;

    watch_result(watch_result&&) = default;

    ~watch_result() noexcept;

    /** The initial result of the fetch. **/
    const get_result& initial() const & { return _initial; }
    get_result&       initial() &       { return _initial; }
    get_result        initial() &&      { return std::move(_initial); }

    /** Future to be delivered when the watch is triggered. **/
    const future<event>& next() const & { return _next; }
    future<event>&       next() &       { return _next; }
    future<event>        next() &&      { return std::move(_next); }

private:
    get_result    _initial;
    future<event> _next;
};

std::ostream& operator<<(std::ostream&, const watch_result&);

std::string to_string(const watch_result&);

}
