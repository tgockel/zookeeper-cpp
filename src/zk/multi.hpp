#pragma once

#include <zk/config.hpp>

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <vector>

#include "acl.hpp"
#include "buffer.hpp"
#include "forwards.hpp"
#include "types.hpp"

namespace zk
{

/** \addtogroup Client
 *  \{
**/

enum class op_type : int
{
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
    struct check_data
    {
        std::string path;
        version     check;

        explicit check_data(std::string path, version check);
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
    };

    /// \see client::create
    static op create(std::string path, buffer data, acl rules, create_mode mode = create_mode::normal);
    static op create(std::string path, buffer data, create_mode mode = create_mode::normal);

    struct erase_data
    {
        std::string path;
        version     check;

        explicit erase_data(std::string path, version check);
    };

    /// \see client::erase
    static op erase(std::string path, version check = version::any());

    struct set_data
    {
        std::string path;
        buffer      data;
        version     check;

        explicit set_data(std::string path, buffer data, version check);
    };

    /// \see client::set
    static op set(std::string path, buffer data, version check = version::any());

public:
    op(const op&);
    op(op&&) noexcept;

    op& operator=(const op&) = delete;
    op& operator=(op&&) = delete;

    ~op() noexcept;

    op_type type() const { return _type; }

    const check_data& as_check() const;

    const create_data& as_create() const;

    const erase_data& as_erase() const;

    const set_data& as_set() const;

private:
    union any_data
    {
        check_data  check;
        create_data create;
        erase_data  erase;
        set_data    set;

        any_data(std::nullptr_t) noexcept;
        any_data(check_data&&) noexcept;
        any_data(create_data&&) noexcept;
        any_data(erase_data&&) noexcept;
        any_data(set_data&&) noexcept;

        ~any_data() noexcept;
    };

    explicit op(check_data&&) noexcept;
    explicit op(create_data&&) noexcept;
    explicit op(erase_data&&) noexcept;
    explicit op(set_data&&) noexcept;

private:
    op_type  _type;
    any_data _storage;
};

std::ostream& operator<<(std::ostream&, const op&);

std::string to_string(const op&);

class multi_op final
{
public:
    using iterator       = std::vector<op>::iterator;
    using const_iterator = std::vector<op>::const_iterator;
    using size_type      = std::vector<op>::size_type;

public:
    multi_op(std::vector<op> ops) noexcept;

    multi_op(std::initializer_list<op> ops) :
            multi_op(std::vector<op>(ops))
    { }

    ~multi_op() noexcept;

    size_type size() const { return _ops.size(); }

    op&       operator[](size_type idx)       { return _ops[idx]; }
    const op& operator[](size_type idx) const { return _ops[idx]; }

    iterator begin()              { return _ops.begin(); }
    const_iterator begin() const  { return _ops.begin(); }
    const_iterator cbegin() const { return _ops.begin(); }

    iterator end()              { return _ops.end(); }
    const_iterator end() const  { return _ops.end(); }
    const_iterator cend() const { return _ops.end(); }

    void reserve(size_type capacity) { _ops.reserve(capacity); }

    template <typename... TArgs>
    void emplace_back(TArgs&&... args)
    {
        _ops.emplace_back(std::forward<TArgs>(args)...);
    }

    void push_back(op&& x)
    {
        emplace_back(std::move(x));
    }

    void push_back(const op& x)
    {
        emplace_back(x);
    }

private:
    std::vector<op> _ops;
};

std::ostream& operator<<(std::ostream&, const multi_op&);

std::string to_string(const multi_op&);

class multi_result final
{
};

/** \} **/

}
