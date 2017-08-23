/** \file
 *  Describes the various result types of \c client operations.
**/
#pragma once

#include <zk/config.hpp>

#include <iosfwd>
#include <string>
#include <vector>

#include "buffer.hpp"
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

}
