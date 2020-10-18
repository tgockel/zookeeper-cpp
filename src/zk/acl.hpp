#pragma once

#include <zk/config.hpp>

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "forwards.hpp"

namespace zk
{

/// \addtogroup Client
/// \{

/// Describes the ability of a user to perform a certain action. Permissions can be mixed together like integers with
/// \c | and \c &.
enum class permission : unsigned int
{
    none    = 0b00000, //!< No permissions are set (server could have been configured without ACL support).
    read    = 0b00001, //!< You can access the data of a node and can list its children.
    write   = 0b00010, //!< You can set the data of a node.
    create  = 0b00100, //!< You can create a child node.
    erase   = 0b01000, //!< You can erase a child node (but not necessarily this one).
    admin   = 0b10000, //!< You can alter permissions on this node.
    all     = 0b11111, //!< You can do anything.
};

/// \{
/// Set union operation of \ref permission.
inline constexpr permission operator|(permission a, permission b)
{
    return permission(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

inline constexpr permission& operator|=(permission& self, permission mask)
{
    return self = self | mask;
}
/// \}

/// \{
/// Set intersection operation of \ref permission.
inline constexpr permission operator&(permission a, permission b)
{
    return permission(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

inline constexpr permission& operator&=(permission& self, permission mask)
{
    return self = self & mask;
}
/// \}

/// Set inverse operation of \ref permission. This is not exactly the bitwise complement of \a a, as the returned value
/// will only include bits set that are valid in \ref permission discriminants.
inline constexpr permission operator~(permission a)
{
    return permission(~static_cast<unsigned int>(a)) & permission::all;
}

/// Check that \a self allows it \a perform all operations. For example,
/// `allows(permission::read | permission::write, permission::read)` will be \c true, as `read|write` is allowed to
/// \c read.
inline constexpr bool allows(permission self, permission perform)
{
    return (self & perform) == perform;
}

std::ostream& operator<<(std::ostream&, const permission&);

std::string to_string(const permission&);

/// An individual rule in an \ref acl. It consists of a \ref scheme and \ref id pair to identify the who and a
/// \ref permission set to determine what they are allowed to do.
///
/// See <a href="https://zookeeper.apache.org/doc/r3.4.10/zookeeperProgrammers.html#sc_ACLPermissions">"Builtin ACL
/// Schemes"</a> in the ZooKeeper Programmer's Guide for more information.
class acl_rule final
{
public:
    /// Create an ACL under the given \a scheme and \a id with the given \a permissions.
    acl_rule(std::string scheme, std::string id, permission permissions);

    acl_rule(const acl_rule&)            = default;
    acl_rule& operator=(const acl_rule&) = default;

    acl_rule(acl_rule&&)            = default;
    acl_rule& operator=(acl_rule&&) = default;

    ~acl_rule() noexcept;

    /// The authentication scheme this list is used for. The most common scheme is `"auth"`, which allows any
    /// authenticated user to perform actions (see \ref acls::creator_all).
    ///
    /// ZooKeeper's authentication system is extensible, but the majority of use cases are covered by the built-in
    /// schemes:
    ///
    /// - \c "world" -- This has a single ID \c "anyone" that represents any user of the system. The ACLs
    ///   \ref acls::open_unsafe and \ref acls::read_unsafe use the \c "world" scheme.
    /// - \c "auth" -- This represents any authenticated user. The \c id field is unused. The ACL \ref acls::creator_all
    ///   uses the \c "auth" scheme.
    /// - \c "digest" -- This uses a \c "${username}:${password}" string to generate MD5 hash which is then used as an
    ///   identity. Authentication is done by sending the string in clear text. When used in the ACL, the expression
    ///   will be the \c "${username}:${digest}", where \c digest is the base 64 encoded SHA1 digest of \c password.
    /// - \c "ip" -- This uses the client host IP as an identity. The \c id expression is an IP address or CIDR netmask,
    ///   which will be matched against the client identity.
    const std::string& scheme() const
    {
        return _scheme;
    }

    /// The ID of the user under the \ref scheme. For example, with the \c "ip" \c scheme, this is an IP address or CIDR
    /// netmask.
    const std::string& id() const
    {
        return _id;
    }

    /// The permissions associated with this ACL.
    const permission& permissions() const
    {
        return _permissions;
    }

private:
    std::string _scheme;
    std::string _id;
    permission  _permissions;
};

/// Compute a hash for the given \a rule.
std::size_t hash(const acl_rule& rule);

[[gnu::pure]] bool operator==(const acl_rule& lhs, const acl_rule& rhs);
[[gnu::pure]] bool operator!=(const acl_rule& lhs, const acl_rule& rhs);
[[gnu::pure]] bool operator< (const acl_rule& lhs, const acl_rule& rhs);
[[gnu::pure]] bool operator<=(const acl_rule& lhs, const acl_rule& rhs);
[[gnu::pure]] bool operator> (const acl_rule& lhs, const acl_rule& rhs);
[[gnu::pure]] bool operator>=(const acl_rule& lhs, const acl_rule& rhs);

std::ostream& operator<<(std::ostream&, const acl_rule&);

std::string to_string(const acl_rule&);

/// An access control list is a wrapper around \ref acl_rule instances. In general, the ACL system is similar to UNIX
/// file access permissions, where znodes act as files. Unlike UNIX, each znode can have any number of ACLs to
/// correspond with the potentially limitless (and pluggable) authentication schemes. A more surprising difference is
/// that ACLs are not recursive: If \c /path is only readable by a single user, but \c /path/sub is world-readable, then
/// anyone will be able to read \c /path/sub.
///
/// See <a href="https://zookeeper.apache.org/doc/trunk/zookeeperProgrammers.html#sc_ZooKeeperAccessControl">ZooKeeper
/// Programmer's Guide</a> for more information.
///
/// \see acls
class acl final
{
public:
    using iterator       = std::vector<acl_rule>::iterator;
    using const_iterator = std::vector<acl_rule>::const_iterator;
    using size_type      = std::size_t;

public:
    /// Create an empty ACL. Keep in mind that an empty ACL is an illegal ACL.
    acl() = default;

    /// Create an instance with the provided \a rules.
    acl(std::vector<acl_rule> rules) noexcept;

    /// Create an instance with the provided \a rules.
    acl(std::initializer_list<acl_rule> rules) :
            acl(std::vector<acl_rule>(rules))
    { }

    acl(const acl&)            = default;
    acl& operator=(const acl&) = default;

    acl(acl&&)            = default;
    acl& operator=(acl&&) = default;

    ~acl() noexcept;

    /// The number of rules in this ACL.
    size_type size() const { return _impl.size(); }

    /// \{
    /// Get the rule at the given \a idx.
    const acl_rule& operator[](size_type idx) const { return _impl[idx]; }
    acl_rule&       operator[](size_type idx)       { return _impl[idx]; }
    /// \}

    /// \{
    /// Get the rule at the given \a idx.
    ///
    /// \throws std::out_of_range if the \a idx is larger than \ref size.
    const acl_rule& at(size_type idx) const { return _impl.at(idx); }
    acl_rule&       at(size_type idx)       { return _impl.at(idx); }
    /// \}

    /// \{
    /// Get an iterator to the beginning of the rule list.
    iterator begin()              { return _impl.begin(); }
    const_iterator begin() const  { return _impl.begin(); }
    const_iterator cbegin() const { return _impl.begin(); }
    /// \}

    /// \{
    /// Get an iterator to the end of the rule list.
    iterator end()              { return _impl.end(); }
    const_iterator end() const  { return _impl.end(); }
    const_iterator cend() const { return _impl.end(); }
    /// \}

    /// Increase the reserved memory block so it can store at least \a capacity rules without reallocating.
    void reserve(size_type capacity) { _impl.reserve(capacity); }

    /// Construct a rule emplace on the end of the list using \a args.
    ///
    /// \see push_back
    template <typename... TArgs>
    void emplace_back(TArgs&&... args)
    {
        _impl.emplace_back(std::forward<TArgs>(args)...);
    }

    /// \{
    /// Add the rule \a x to the end of this list.
    void push_back(acl_rule&& x)      { emplace_back(std::move(x)); }
    void push_back(const acl_rule& x) { emplace_back(x); }
    /// \}

private:
    std::vector<acl_rule> _impl;
};

[[gnu::pure]] bool operator==(const acl& lhs, const acl& rhs);
[[gnu::pure]] bool operator!=(const acl& lhs, const acl& rhs);

std::ostream& operator<<(std::ostream&, const acl&);

std::string to_string(const acl& self);

/// Commonly-used ACLs.
class acls
{
public:
    /// This ACL gives the creators authentication id's all permissions.
    static const acl& creator_all();

    /// This is a completely open ACL. It is also the ACL used in operations like \ref client::create if no ACL is
    /// specified.
    static const acl& open_unsafe();

    /// This ACL gives the world the ability to read.
    static const acl& read_unsafe();
};

/// \}

}

namespace std
{

template <>
class hash<zk::acl_rule>
{
public:
    using argument_type = zk::acl_rule;
    using result_type   = std::size_t;

    result_type operator()(const argument_type& x) const
    {
        return zk::hash(x);
    }
};

}
