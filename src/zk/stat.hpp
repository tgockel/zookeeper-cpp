#pragma once

#include <zk/config.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>

namespace zk
{

/** Base type for creating strong ID types. **/
template <typename TReal, typename TId>
struct strong_id
{
    using value_type = TId;

    value_type value;

    strong_id() noexcept = default;

    constexpr explicit strong_id(value_type val) noexcept :
            value(val)
    { }

    constexpr value_type get() const noexcept { return value; }

    explicit constexpr operator value_type() noexcept { return value; }

    TReal& operator++()
    {
        ++this->value;
        return *static_cast<TReal*>(this);
    }

    TReal operator++(int)
    {
        TReal copy(*this);
        operator++();
        return copy;
    }

    TReal& operator--()
    {
        --this->value;
        return *static_cast<TReal*>(this);;
    }

    TReal operator--(int)
    {
        TReal copy(*this);
        operator--();
        return copy;
    }
};

template <typename TReal, typename TId>
constexpr bool operator==(const strong_id<TReal, TId>& a, const strong_id<TReal, TId>& b)
{
    return a.value == b.value;
}

template <typename TReal, typename TId>
constexpr bool operator!=(const strong_id<TReal, TId>& a, const strong_id<TReal, TId>& b)
{
    return a.value != b.value;
}

template <typename TReal, typename TId>
constexpr bool operator<(const strong_id<TReal, TId>& a, const strong_id<TReal, TId>& b)
{
    return a.value < b.value;
}

template <typename TReal, typename TId>
constexpr bool operator<=(const strong_id<TReal, TId>& a, const strong_id<TReal, TId>& b)
{
    return a.value <= b.value;
}

template <typename TReal, typename TId>
constexpr bool operator>(const strong_id<TReal, TId>& a, const strong_id<TReal, TId>& b)
{
    return a.value > b.value;
}

template <typename TReal, typename TId>
constexpr bool operator>=(const strong_id<TReal, TId>& a, const strong_id<TReal, TId>& b)
{
    return a.value >= b.value;
}

template <typename TReal, typename TId>
inline std::size_t hash(const strong_id<TReal, TId>& x)
{
    return std::hash<TId>()(x.value);
}

struct version final :
        public strong_id<version, std::int32_t>
{
    /** An invalid version specifier. This will never be returned by the database and will always be rejected in commit
     *  operations. This is a good value to use as a placeholder when you are searching for the proper \c version.
    **/
    static constexpr version invalid() { return version(0); };

    /** When specified in an operation, this version specifier will always pass. It is the equivalent to not performing
     *  a version check.
    **/
    static constexpr version any() { return version(-1); };

    using strong_id::strong_id;
};

std::ostream& operator<<(std::ostream&, const version&);

std::string to_string(const version&);

struct transaction_id final :
        public strong_id<transaction_id, std::size_t>
{
    using strong_id::strong_id;
};

std::ostream& operator<<(std::ostream&, const transaction_id&);

std::string to_string(const transaction_id&);

/** Statistics about a znode, similar to the UNIX `stat` structure.
 *
 *  \note{Time in ZooKeeper}
 *  The concept of time is tricky in distributed systems. ZooKeeper keeps track of time in a number of ways.
 *
 *  - **zxid**: Every change to a ZooKeeper cluster receives a stamp in the form of a *zxid* (ZooKeeper Transaction ID).
 *    This exposes the total ordering of all changes to ZooKeeper. Each change will have a unique *zxid* -- if *zxid:a*
 *    is smaller than *zxid:b*, then the associated change to *zxid:a* happened before *zxid:b*.
 *  - **Version Numbers**: Every change to a znode will cause an increase to one of the version numbers of that node.
 *  - **Clock Time**: ZooKeeper does not use clock time to make decisions, but it uses it to put timestamps into the
 *    \c stat structure.
**/
struct stat final
{
public:
    using time_point = std::chrono::system_clock::time_point;

public:
    /** The transaction ID that created the znode. **/
    transaction_id create_transaction;

    /** The last transaction that modified the znode. **/
    transaction_id modified_transaction;

    /** The transaction ID that last modified the children of the znode. **/
    transaction_id child_modified_transaction;

    /** Time the znode was created.
     *
     *  \warning
     *  This should \e not be relied on for any logic. ZooKeeper sets this time based on the system clock of the master
     *  server at the time the znode is created and performs no validity checking or synchronization with other servers
     *  in the cluster. As such, there is no guarantee that this value is accurrate. There are many situations where a
     *  znode with a higher \c create_transaction (created after) will have a lower \c create_time (appear to have been
     *  created before).
    **/
    time_point create_time;

    /** Last time the znode was last modified. Like \c create_time, this is not a reliable source. **/
    time_point modified_time;

    /** The number of changes to the data of the znode. **/
    version data_version;

    /** The number of changes to the children of the znode. **/
    version child_version;

    /** The number of changes to the ACL of the znode. **/
    version acl_version;

    /** The session ID of the owner of this znode, if it is an ephemeral entry. **/
    std::uint64_t ephemeral_owner;

    /** The size of the data field of the znode. **/
    std::size_t data_size;

    /** The number of children this znode has. **/
    std::size_t children_count;

    /** Is the znode an ephemeral entry? **/
    bool is_ephemeral() const
    {
        return ephemeral_owner == 0U;
    }
};

std::ostream& operator<<(std::ostream&, const stat&);

std::string to_string(const stat&);

}

namespace std
{

template <>
struct hash<zk::version>
{
    using argument_type = zk::version;
    using result_type   = std::size_t;

    result_type operator()(const argument_type& x) const
    {
        return zk::hash(x);
    }
};

template <>
struct hash<zk::transaction_id>
{
    using argument_type = zk::transaction_id;
    using result_type   = std::size_t;

    result_type operator()(const argument_type& x) const
    {
        return zk::hash(x);
    }
};

}
