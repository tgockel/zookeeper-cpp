#pragma once

#include <zk/config.hpp>

#include <chrono>
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

/** Base type for version types. These are distinct so we can emit a compilation error on attempts to use the incorrect
 *  version type (for example, \c client::set_acl does a check on \c acl_version instead of the standard \c version).
 *
 *  \see version
 *  \see acl_version
 *  \see child_version
**/
template <typename TReal>
struct basic_version :
        public strong_id<TReal, std::int32_t>
{
    /** An invalid version specifier. This will never be returned by the database and will always be rejected in commit
     *  operations. This is a good value to use as a placeholder when you are searching for the proper \c version.
    **/
    static constexpr TReal invalid() { return TReal(-42); };

    /** When specified in an operation, this version specifier will always pass. It is the equivalent to not performing
     *  a version check.
    **/
    static constexpr TReal any() { return TReal(-1); };

    using strong_id<TReal, std::int32_t>::strong_id;
};

/** Represents a version of the data.
 *
 *  \see stat::data_version
**/
struct version final :
        public basic_version<version>
{
    using basic_version<version>::basic_version;
};

std::ostream& operator<<(std::ostream&, const version&);

std::string to_string(const version&);

/** Represents a version of the ACL of a ZNode.
 *
 *  \see stat::acl_version
**/
struct acl_version final :
        public basic_version<acl_version>
{
    using basic_version<acl_version>::basic_version;
};

std::ostream& operator<<(std::ostream&, const acl_version&);

std::string to_string(const acl_version&);

/** Represents a version of the children of a ZNode.
 *
 *  \see stat::child_version
**/
struct child_version final :
        public basic_version<child_version>
{
    using basic_version<child_version>::basic_version;
};

std::ostream& operator<<(std::ostream&, const child_version&);

std::string to_string(const child_version&);

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
    zk::version data_version;

    /** The number of changes to the children of the znode. **/
    zk::child_version child_version;

    /** The number of changes to the ACL of the znode. **/
    zk::acl_version acl_version;

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

/** When used in \c client::set, this value determines how the znode is created on the server. These values can be ORed
 *  together to create combinations.
**/
enum class create_mode : unsigned int
{
    /** Standard behavior of a znode -- the opposite of doing any of the options. **/
    normal = 0b0000,
    /** The znode will be deleted upon the client's disconnect. **/
    ephemeral = 0b0001,
    /** The name of the znode will be appended with a monotonically increasing number. The actual path name of a
     *  sequential node will be the given path plus a suffix \c "i" where \c i is the current sequential number of the
     *  node. The sequence number is always fixed length of 10 digits, 0 padded. Once such a node is created, the
     *  sequential number will be incremented by one.
    **/
    sequential = 0b0010,
    /** Container nodes are special purpose nodes useful for recipes such as leader, lock, etc. When the last child of a
     *  container is deleted, the container becomes a candidate to be deleted by the server at some point in the future.
     *  Given this property, you should be prepared to get \c no_node when creating children inside of this container
     *  node.
    **/
    container = 0b0100,
};

constexpr create_mode operator|(create_mode a, create_mode b)
{
    return create_mode(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

constexpr create_mode operator&(create_mode a, create_mode b)
{
    return create_mode(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

constexpr create_mode operator~(create_mode a)
{
    return create_mode(~static_cast<unsigned int>(a));
}

/** Check that \a self has \a flags set. **/
constexpr bool is_set(create_mode self, create_mode flags)
{
    return (self & flags) == flags;
}

std::ostream& operator<<(std::ostream&, const create_mode&);

std::string to_string(const create_mode&);

/** Enumeration of types of events that may occur. **/
enum class event_type : int
{
    error           =  0, //!< Invalid event (this should never be issued).
    created         =  1, //!< Issued when a znode at a given path is created.
    erased          =  2, //!< Issued when a znode at a given path is erased.
    changed         =  3, //!< Issued when the data of a watched znode are altered. This event value is issued whenever
                          //!< a \e set operation occurs without an actual contents check, so there is no guarantee the
                          //!< data actually changed.
    child           =  4, //!< Issued when the children of a watched znode are created or deleted. This event is not
                          //!< issued when the data within children is altered.
    session         = -1, //!< This value is issued as part of an event when the \c state changes.
    not_watching    = -2, //!< Watch has been forcefully removed. This is generated when the server for some reason
                          //!< (probably a resource constraint), will no longer watch a node for a client.
};

std::ostream& operator<<(std::ostream&, const event_type&);

std::string to_string(const event_type&);

/** Enumeration of states the client may be at when a watch triggers. It represents the state of the connection at the
 *  time the event was generated.
**/
enum class state : int
{
    closed                =    0, //!< The client is not connected to any server in the ensemble.
    connecting            =    1, //!< The client is connecting.
    associating           =    2, //!< Client is attempting to associate a session.
    connected             =    3, //!< The client is in the connected state -- it is connected to a server in the
                                  //!< ensemble (one of the servers specified in the host connection parameter during
                                  //!< ZooKeeper client creation).
    read_only             =    5, //!< The client is connected to a read-only server, that is the server which is not
                                  //!< currently connected to the majority. The only operations allowed after receiving
                                  //!< this state is read operations. This state is generated for read-only clients only
                                  //!< since read/write clients aren't allowed to connect to read-only servers.
    not_connected         =  999,
    expired_session       = -112, //!< The serving cluster has expired this session. The ZooKeeper client connection
                                  //!< (the session) is no longer valid. You must create a new client \c connection if
                                  //!< you with to access the ensemble.
    authentication_failed = -113, //!< Authentication has failed -- connection requires a new \c connection instance
                                  //!< with different credentials.
};

std::ostream& operator<<(std::ostream&, const state&);

std::string to_string(const state&);

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
