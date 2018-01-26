#pragma once

#include <zk/config.hpp>

#include <iosfwd>
#include <string>
#include <variant>

#include "acl.hpp"
#include "buffer.hpp"
#include "types.hpp"

namespace zk
{

/** \addtogroup Client
 *  \{
**/

enum class op_type : int
{
    get,
    check,
    create,
    erase,
    set,
};

std::ostream& operator<<(std::ostream&, const op_type&);

std::string to_string(const op_type&);

class op final
{
public:
    struct get_data
    {
        std::string path;

        explicit get_data(std::string path);

        op_type type() const { return op_type::get; }
    };

    /// \see client::get
    static op get(std::string path);

    /// \see client::get_children
    static op get_children(std::string path);

    /// \see client::exists
    static op exists(std::string path);

    /// \see client::get_acl
    static op get_acl(std::string path);

    /// \see client::set_acl
    static op set_acl(std::string path, acl rules, acl_version check);

    struct check_data
    {
        std::string path;
        version     check;

        explicit check_data(std::string path, version check);

        op_type type() const { return op_type::check; }
    };

    /** Check that the given \a path exists with the provided version \a check (which can be \c version::any). **/
    static op check(std::string path, version check = version::any());

    struct create_data
    {
        std::string path;
        buffer      data;
        acl         rules;
        create_mode mode;

        explicit create_data(std::string path, buffer data, acl rules, create_mode mode);

        op_type type() const { return op_type::create; }
    };

    /// \see client::create
    static op create(std::string path, buffer data, acl rules, create_mode mode = create_mode::normal);
    static op create(std::string path, buffer data, create_mode mode = create_mode::normal);

    struct erase_data
    {
        std::string path;
        version     check;

        explicit erase_data(std::string path, version check);

        op_type type() const { return op_type::erase; }
    };

    /// \see client::erase
    static op erase(std::string path, version check = version::any());

    struct set_data
    {
        std::string path;
        buffer      data;
        version     check;

        explicit set_data(std::string path, buffer data, version check);

        op_type type() const { return op_type::set; }
    };

    /// \see client::set
    static op set(std::string path, buffer data, version check = version::any());

public:
    op(const op&);
    op(op&&) noexcept;

    op& operator=(const op&) = delete;
    op& operator=(op&&) = delete;

    ~op() noexcept;

    op_type type() const;

    const get_data& as_get() const;

    const check_data& as_check() const;

    const create_data& as_create() const;

    const erase_data& as_erase() const;

    const set_data& as_set() const;

private:
    using any_data = std::variant<get_data, check_data, create_data, erase_data, set_data>;

    explicit op(any_data&&) noexcept;

    template <typename T>
    const T& as(ptr<const char> operation) const;

    friend std::ostream& operator<<(std::ostream&, const op&);

private:
    any_data _storage;
};

std::ostream& operator<<(std::ostream&, const op&);

std::string to_string(const op&);

/** \} **/

}
