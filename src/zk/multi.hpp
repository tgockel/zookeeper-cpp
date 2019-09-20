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

/// \addtogroup Client
/// \{

/// Describes the type of an \ref op.
enum class op_type : int
{
    check,  //!< \ref op::check
    create, //!< \ref op::create
    erase,  //!< \ref op::erase
    set,    //!< \ref op::set
};

std::ostream& operator<<(std::ostream&, const op_type&);

std::string to_string(const op_type&);

/// Represents a single operation of a \ref multi_op.
class op final
{
public:
    /// Data for a \ref op::check operation.
    struct check_data
    {
        std::string path;
        version     check;

        explicit check_data(std::string path, version check);

        op_type type() const { return op_type::check; }
    };

    /// Check that the given \a path exists with the provided version \a check (which can be \c version::any).
    static op check(std::string path, version check = version::any());

    /// Data for a \ref op::create operation.
    struct create_data
    {
        std::string path;
        buffer      data;
        acl         rules;
        create_mode mode;

        explicit create_data(std::string path, buffer data, acl rules, create_mode mode);

        op_type type() const { return op_type::create; }
    };

    /// \{
    /// Create a new entry at the given \a path with the \a data.
    ///
    /// \see client::create
    static op create(std::string path, buffer data, acl rules, create_mode mode = create_mode::normal);
    static op create(std::string path, buffer data, create_mode mode = create_mode::normal);
    /// \}

    /// Data for a \ref op::erase operation.
    struct erase_data
    {
        std::string path;
        version     check;

        explicit erase_data(std::string path, version check);

        op_type type() const { return op_type::erase; }
    };

    /// Delete the entry at the given \a path if it matches the version \a check.
    ///
    /// \see client::erase
    static op erase(std::string path, version check = version::any());

    /// Data for a \ref op::set operation.
    struct set_data
    {
        std::string path;
        buffer      data;
        version     check;

        explicit set_data(std::string path, buffer data, version check);

        op_type type() const { return op_type::set; }
    };

    /// Set the \a data for the entry at \a path if it matches the version \a check.
    ///
    /// \see client::set
    static op set(std::string path, buffer data, version check = version::any());

public:
    op(const op&);
    op(op&&) noexcept;

    op& operator=(const op&) = delete;
    op& operator=(op&&) = delete;

    ~op() noexcept;

    /// Get the underlying type of this operation.
    op_type type() const;

    /// Get the check-specific data.
    ///
    /// \throws std::logic_error if the \ref type is not \ref op_type::check.
    const check_data& as_check() const;

    /// Get the create-specific data.
    ///
    /// \throws std::logic_error if the \ref type is not \ref op_type::create.
    const create_data& as_create() const;

    /// Get the erase-specific data.
    ///
    /// \throws std::logic_error if the \ref type is not \ref op_type::erase.
    const erase_data& as_erase() const;

    /// Get the set-specific data.
    ///
    /// \throws std::logic_error if the \ref type is not \ref op_type::set.
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

/// A collection of operations that will be performed atomically.
///
/// \see client::commit
/// \see op
class multi_op final
{
public:
    using iterator       = std::vector<op>::iterator;
    using const_iterator = std::vector<op>::const_iterator;
    using size_type      = std::vector<op>::size_type;

public:
    /// Create an empty operation set.
    multi_op() noexcept
    { }

    /// Create an instance from the provided \a ops.
    multi_op(std::vector<op> ops) noexcept;

    /// Create an instance from the provided \a ops.
    multi_op(std::initializer_list<op> ops) :
            multi_op(std::vector<op>(ops))
    { }

    ~multi_op() noexcept;

    /// The number of operations in this transaction bundle.
    size_type size() const { return _ops.size(); }

    /// \{
    /// Get the operation at the given \a idx.
    const op& operator[](size_type idx) const { return _ops[idx]; }
    op&       operator[](size_type idx)       { return _ops[idx]; }
    /// \}

    /// \{
    /// Get the operation at the given \a idx.
    ///
    /// \throws std::out_of_range if \a idx is larger than \ref size.
    const op& at(size_type idx) const { return _ops.at(idx); }
    op&       at(size_type idx)       { return _ops.at(idx); }
    /// \}

    /// \{
    /// Get an iterator to the beginning of the operation list.
    iterator begin()              { return _ops.begin(); }
    const_iterator begin() const  { return _ops.begin(); }
    const_iterator cbegin() const { return _ops.begin(); }
    /// \}

    /// \{
    /// Get an iterator to the end of the operation list.
    iterator end()              { return _ops.end(); }
    const_iterator end() const  { return _ops.end(); }
    const_iterator cend() const { return _ops.end(); }
    /// \}

    /// Increase the reserved memory block so it can store at least \a capacity operations without reallocating.
    void reserve(size_type capacity) { _ops.reserve(capacity); }

    /// Construct an operation emplace on the end of the list using \a args.
    ///
    /// \see push_back
    template <typename... TArgs>
    void emplace_back(TArgs&&... args)
    {
        _ops.emplace_back(std::forward<TArgs>(args)...);
    }

    /// \{
    /// Add the operation \a x to the end of this list.
    void push_back(op&&      x) { emplace_back(std::move(x)); }
    void push_back(const op& x) { emplace_back(x); }
    /// \}

private:
    std::vector<op> _ops;
};

std::ostream& operator<<(std::ostream&, const multi_op&);

std::string to_string(const multi_op&);

/// The result of a successful \ref client::commit operation.
class multi_result final
{
public:
    /// A part of a result. The behavior depends on the type of \ref op provided to the original transaction.
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

        /// The \ref op_type of the \ref op that caused this result.
        op_type type() const { return _type; }

        /// Get the create-specific result data.
        ///
        /// \throws std::logic_error if the \ref type is not \ref op_type::set.
        const create_result& as_create() const;

        /// Get the set-specific result data.
        ///
        /// \throws std::logic_error if the \ref type is not \ref op_type::set.
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

    multi_result(multi_result&&) = default;
    multi_result & operator=(multi_result&&) = default;

    ~multi_result() noexcept;

    /// The number of results in this transaction bundle.
    size_type size() const { return _parts.size(); }

    /// \{
    /// Get the result at the given \a idx.
    ///
    /// \throws std::out_of_range if \a idx is larger than \a size.
    part&       operator[](size_type idx)       { return _parts[idx]; }
    const part& operator[](size_type idx) const { return _parts[idx]; }
    /// \}

    /// \{
    /// Get the result at the given \a idx.
    ///
    /// \throws std::out_of_range if \a idx is larger than \ref size.
    const part& at(size_type idx) const { return _parts.at(idx); }
    part&       at(size_type idx)       { return _parts.at(idx); }
    /// \}

    /// \{
    /// Get an iterator to the beginning of the result list.
    iterator begin()              { return _parts.begin(); }
    const_iterator begin() const  { return _parts.begin(); }
    const_iterator cbegin() const { return _parts.begin(); }
    /// \}

    /// \{
    /// Get an iterator to the end of the result list.
    iterator end()              { return _parts.end(); }
    const_iterator end() const  { return _parts.end(); }
    const_iterator cend() const { return _parts.end(); }
    /// \}

    /// Increase the reserved memory block so it can store at least \a capacity results without reallocating.
    void reserve(size_type capacity) { _parts.reserve(capacity); }

    /// Construct a result emplace on the end of the list using \a args.
    ///
    /// \see push_back
    template <typename... TArgs>
    void emplace_back(TArgs&&... args)
    {
        _parts.emplace_back(std::forward<TArgs>(args)...);
    }

    /// \{
    /// Add the operation \a x to the end of this list.
    void push_back(part&& x)      { emplace_back(std::move(x)); }
    void push_back(const part& x) { emplace_back(x); }
    /// \}

private:
    std::vector<part> _parts;
};

std::ostream& operator<<(std::ostream&, const multi_result::part&);
std::ostream& operator<<(std::ostream&, const multi_result&);

std::string to_string(const multi_result::part&);
std::string to_string(const multi_result&);

/** \} **/

}
