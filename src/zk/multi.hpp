#pragma once

#include <zk/config.hpp>

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

#include "acl.hpp"
#include "buffer.hpp"
#include "forwards.hpp"
#include "results.hpp"
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

    const check_data& as_check() const;

    const create_data& as_create() const;

    const erase_data& as_erase() const;

    const set_data& as_set() const;

private:
    using any_data = std::variant<check_data, create_data, erase_data, set_data>;

    explicit op(any_data&&) noexcept;

    template <typename T>
    const T& as(ptr<const char> operation) const;

    friend std::ostream& operator<<(std::ostream&, const op&);

private:
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
    multi_op() noexcept
    { }

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
public:
    /** A part of a result. The behavior depends on the \c op_type of \c op provided to the original transaction. **/
    class part final
    {
    public:
        explicit part(op_type, std::nullptr_t) noexcept;
        explicit part(create_result) noexcept;
        explicit part(set_result) noexcept;

        part(const part&);
        part(part&&) noexcept;

        part& operator=(const part&) = delete;
        part& operator=(part&&) = delete;

        ~part() noexcept;

        /** The \c op_type of the \c op that caused this result. **/
        op_type type() const { return _type; }

        const create_result& as_create() const;

        const set_result& as_set() const;

    private:
        using any_result = std::variant<std::monostate, create_result, set_result>;

        template <typename T>
        const T& as(ptr<const char> operation) const;

    private:
        op_type    _type;
        any_result _storage;
    };

    using iterator       = std::vector<part>::iterator;
    using const_iterator = std::vector<part>::const_iterator;
    using size_type      = std::vector<part>::size_type;

public:
    multi_result() noexcept
    { }

    multi_result(std::vector<part> parts) noexcept;

    ~multi_result() noexcept;

    size_type size() const { return _parts.size(); }

    part&       operator[](size_type idx)       { return _parts[idx]; }
    const part& operator[](size_type idx) const { return _parts[idx]; }

    iterator begin()              { return _parts.begin(); }
    const_iterator begin() const  { return _parts.begin(); }
    const_iterator cbegin() const { return _parts.begin(); }

    iterator end()              { return _parts.end(); }
    const_iterator end() const  { return _parts.end(); }
    const_iterator cend() const { return _parts.end(); }

    void reserve(size_type capacity) { _parts.reserve(capacity); }

    template <typename... TArgs>
    void emplace_back(TArgs&&... args)
    {
        _parts.emplace_back(std::forward<TArgs>(args)...);
    }

    void push_back(part&& x)
    {
        emplace_back(std::move(x));
    }

    void push_back(const part& x)
    {
        emplace_back(x);
    }

private:
    std::vector<part> _parts;
};

std::ostream& operator<<(std::ostream&, const multi_result::part&);
std::ostream& operator<<(std::ostream&, const multi_result&);

std::string to_string(const multi_result::part&);
std::string to_string(const multi_result&);

/** \} **/

}
