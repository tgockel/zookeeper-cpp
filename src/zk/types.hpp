#pragma once

#include <zk/config.hpp>

#include <chrono>
#include <iosfwd>
#include <string>

namespace zk
{

/// \addtogroup Client
/// \{

/// Base type for creating strong ID types. These behave similar to a \c typedef, but do not allow conversion between
/// different \c TReal types, even if they share the same \c TId. This makes attempting to use a \c version instead of
/// an \c acl_version in \ref client::set_acl a compile-time failure instead of throwing a \c bad_version at run-time.
///
/// \tparam TReal The "real" leaf type of this ID.
/// \tparam TId The representation type of this ID. This must be some form of integer and is usually \c std::int32_t (as
///  in \ref basic_version).
///
/// \see version
/// \see child_version
/// \see acl_version
/// \see transaction_id
template <typename TReal, typename TId>
struct strong_id
{
    /// The representation type of this ID.
    using value_type = TId;

    /// Underlying value of this ID.
    value_type value;

    /// Default construct the ID. This probably leaves \c value uninitialized (depending on the constructor of
    /// \ref value_type).
    strong_id() noexcept = default;

    /// Construct this instance with the given \a value.
    constexpr explicit strong_id(value_type value) noexcept :
            value(value)
    { }

    /// Get the \ref value of this ID.
    constexpr value_type get() const noexcept { return value; }

    /// Get the \ref value of this ID.
    ///
    /// \see get
    explicit constexpr operator value_type() noexcept { return value; }

    /// \{
    /// Increment the \ref value of this ID by 1. This can be useful for maintaining caches after updates without
    /// getting the result. For example, a transaction with a version-checked set operation knows the next version of
    /// the entry will be only one more.
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
    /// \}

    /// \{
    /// Decrement the \ref value of this ID by 1. This operation is probably not useful, but is included for
    /// completeness.
    TReal& operator--()
    {
        --this->value;
        return *static_cast<TReal*>(this);
    }

    TReal operator--(int)
    {
        TReal copy(*this);
        operator--();
        return copy;
    }
    /// \}
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

/// Compute the \c std::hash of the given \a x.
template <typename TReal, typename TId>
inline std::size_t hash(const strong_id<TReal, TId>& x)
{
    return std::hash<TId>()(x.value);
}

/// Base type for version types. These are distinct so we can emit a compilation error on attempts to use the incorrect
/// version type (for example, \ref client::set_acl does a check on \ref acl_version instead of the standard
/// \ref version).
///
/// \see version
/// \see acl_version
/// \see child_version
template <typename TReal>
struct basic_version :
        public strong_id<TReal, std::int32_t>
{
    /// An invalid version specifier. This will never be returned by the database and will always be rejected in commit
    /// operations. This is a good value to use as a placeholder when you are searching for the proper \ref version.
    static constexpr TReal invalid() { return TReal(-42); };

    /** When specified in an operation, this version specifier will always pass. It is the equivalent to not performing
     *  a version check.
    **/
    static constexpr TReal any() { return TReal(-1); };

    using strong_id<TReal, std::int32_t>::strong_id;
};

/// Represents a version of the data.
///
/// \see stat::data_version
struct version final :
        public basic_version<version>
{
    using basic_version<version>::basic_version;
};

std::ostream& operator<<(std::ostream&, const version&);

std::string to_string(const version&);

/// Represents a version of the ACL of an entry.
///
/// \see stat::acl_version
struct acl_version final :
        public basic_version<acl_version>
{
    using basic_version<acl_version>::basic_version;
};

std::ostream& operator<<(std::ostream&, const acl_version&);

std::string to_string(const acl_version&);

/// Represents a version of the children of an entry.
///
/// \see stat::child_version
struct child_version final :
        public basic_version<child_version>
{
    using basic_version<child_version>::basic_version;
};

std::ostream& operator<<(std::ostream&, const child_version&);

std::string to_string(const child_version&);

/// Represents the ZooKeeper transaction ID in which an event happened to an entry.
///
/// \see stat::create_transaction
/// \see stat::modified_transaction
/// \see stat::child_modified_transaction
struct transaction_id final :
        public strong_id<transaction_id, std::size_t>
{
    using strong_id::strong_id;
};

std::ostream& operator<<(std::ostream&, const transaction_id&);

std::string to_string(const transaction_id&);

/// Statistics about a ZooKeeper entry, similar to the UNIX `stat` structure.
///
/// \note{Time in ZooKeeper}
/// The concept of time is tricky in distributed systems. ZooKeeper keeps track of time in a number of ways.
///
/// - **zxid**: Every change to a ZooKeeper cluster receives a stamp in the form of a *zxid* (ZooKeeper Transaction ID).
///   This exposes the total ordering of all changes to ZooKeeper. Each change will have a unique *zxid* -- if *zxid:a*
///   is smaller than *zxid:b*, then the associated change to *zxid:a* happened before *zxid:b*.
/// - **Version Numbers**: Every change to an entry will cause an increase to one of the version numbers of that entry.
/// - **Clock Time**: ZooKeeper does not use clock time to make decisions, but it uses it to put timestamps into the
///   \c stat structure.
struct stat final
{
public:
    using time_point = std::chrono::system_clock::time_point;

public:
    /// The transaction ID that created the entry.
    transaction_id create_transaction;

    /// The last transaction that modified the entry.
    transaction_id modified_transaction;

    /// The transaction ID that last modified the children of the entry.
    transaction_id child_modified_transaction;

    /// Time the entry was created.
    ///
    /// \warning
    /// This should \e not be relied on for any logic. ZooKeeper sets this time based on the system clock of the master
    /// server at the time the entry is created and performs no validity checking or synchronization with other servers
    /// in the cluster. As such, there is no guarantee that this value is accurrate. There are many situations where a
    /// entry with a higher \c create_transaction (created after) will have a lower \c create_time (appear to have been
    /// created before).
    time_point create_time;

    /// Last time the entry was last modified. Like \ref create_time, this is not a reliable source.
    time_point modified_time;

    /// The number of changes to the data of the entry. This value is used in operations like \c client::set that take a
    /// version check before modifying data.
    zk::version data_version;

    /// The number of changes to the children of the entry.
    zk::child_version child_version;

    /// The number of changes to the ACL of the entry.
    zk::acl_version acl_version;

    /// The session ID of the owner of this entry, if it is an ephemeral entry. In general, this is not useful beyond a
    /// check for being \c 0.
    ///
    /// \see is_ephemeral
    std::uint64_t ephemeral_owner;

    /// The size of the data field of the entry.
    std::size_t data_size;

    /// The number of children this entry has.
    std::size_t children_count;

    /// Is the entry an ephemeral entry?
    bool is_ephemeral() const
    {
        return ephemeral_owner == 0U;
    }
};

std::ostream& operator<<(std::ostream&, const stat&);

std::string to_string(const stat&);

/// When used in \c client::set, this value determines how the entry is created on the server. These values can be ORed
/// together to create combinations.
enum class create_mode : unsigned int
{
    /// Standard behavior of an entry -- the opposite of doing any of the options.
    normal = 0b0000,
    /// The entry will be deleted when the client session expires.
    ephemeral = 0b0001,
    /// The name of the entry will be appended with a monotonically increasing number. The actual path name of a
    /// sequential entry will be the given path plus a suffix \c "i" where \c i is the current sequential number of the
    /// entry. The sequence number is always fixed length of 10 digits, 0 padded. Once such a entry is created, the
    /// sequential number will be incremented by one.
    sequential = 0b0010,
    /// Container entries are special purpose entries useful for recipes such as leader, lock, etc. When the last child
    /// of a container is deleted, the container becomes a candidate to be deleted by the server at some point in the
    /// future. Given this property, you should be prepared to get \ref no_entry when creating children inside of this
    /// container entry.
    container = 0b0100,
};

/// Set union operation of \ref create_mode.
constexpr create_mode operator|(create_mode a, create_mode b)
{
    return create_mode(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

/// Set intersection operation of \ref create_mode.
constexpr create_mode operator&(create_mode a, create_mode b)
{
    return create_mode(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

/// Set inverse operation of \ref create_mode. This is not exactly the bitwise complement of \a a, as the returned value
/// will only include bits set that are valid in \ref create_mode discriminants.
constexpr create_mode operator~(create_mode a)
{
    return create_mode(~static_cast<unsigned int>(a) & 0b0111);
}

/// Check that \a self has \a flags set.
constexpr bool is_set(create_mode self, create_mode flags)
{
    return (self & flags) == flags;
}

std::ostream& operator<<(std::ostream&, const create_mode&);

std::string to_string(const create_mode&);

/// Enumeration of types of events that may occur.
enum class event_type : int
{
    error           =  0, //!< Invalid event (this should never be issued).
    created         =  1, //!< Issued when an entry for a given path is created.
    erased          =  2, //!< Issued when an entry at a given path is erased.
    changed         =  3, //!< Issued when the data of a watched entry is altered. This event value is issued whenever
                          //!< a \e set operation occurs without an actual contents check, so there is no guarantee the
                          //!< data actually changed.
    child           =  4, //!< Issued when the children of a watched entry are created or deleted. This event is not
                          //!< issued when the data within children is altered.
    session         = -1, //!< This value is issued as part of an event when the \c state changes.
    not_watching    = -2, //!< Watch has been forcefully removed. This is generated when the server for some reason
                          //!< (probably a resource constraint), will no longer watch an entry for a client.
};

std::ostream& operator<<(std::ostream&, const event_type&);

std::string to_string(const event_type&);

/// Enumeration of states the client may be at when a watch triggers. It represents the state of the connection at the
/// time the event was generated.
///
/// \dot
/// digraph G {
///   rankdir = LR
///
///   connecting
///   connected
///   read_only
///   authentication_failed
///   expired_session
///   closed
///
///   connecting -> connected             [label="Successful connection"]
///   connecting -> read_only             [label="Connection to read-only peer"]
///   connecting -> authentication_failed [label="Authentication failure"]
///   connecting -> expired_session       [label="Session lost"]
///   connecting -> closed                [label="close()"]
///   connected -> connecting             [label="Connection lost" color="red"]
///   connected -> closed                 [label="close()"]
///   read_only -> connecting             [label="Connection lost" color="red"]
///   read_only -> closed                 [label="close()"]
///   authentication_failed -> closed     [label="close()"]
///   expired_session -> closed           [label="close()"]
/// }
/// \enddot
///
/// \note
/// If you are familiar with the C API, notably missing from this list is a \c ZOO_NOTCONNECTED_STATE equivalent. This
/// state happens in cases where the client disconnects on purpose (either on initial connection or just after ensemble
/// reconfiguration). However, the ability to see this state is limited to times when you call \c zk_state at just the
/// right moment. This state leads to a bit of confusion with \c closed and \c expired_session, so it is not in the
/// list. Instead, these cases are presented as just \c connecting, as the client is attempting to reconnect to the
/// cluster.
enum class state : int
{
    closed                =    0, //!< The client is not connected to any server in the ensemble.
    connecting            =    1, //!< The client is connecting.
    connected             =    3, //!< The client is in the connected state -- it is connected to a server in the
                                  //!< ensemble (one of the servers specified in the host connection parameter during
                                  //!< ZooKeeper client creation).
    read_only             =    5, //!< The client is connected to a read-only server, that is the server which is not
                                  //!< currently connected to the majority. The only operations allowed after receiving
                                  //!< this state is read operations. This state is generated for read-only clients only
                                  //!< since read/write clients aren't allowed to connect to read-only servers.
    expired_session       = -112, //!< The serving cluster has expired this session. The ZooKeeper client connection
                                  //!< (the session) is no longer valid. You must create a new client \c connection if
                                  //!< you wish to access the ensemble.
    authentication_failed = -113, //!< Authentication has failed -- connection requires a new \c connection instance
                                  //!< with different credentials.
};

std::ostream& operator<<(std::ostream&, const state&);

/// Get the string representation of the provided \a state.
std::string to_string(const state& state);

/// \}

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
struct hash<zk::acl_version>
{
    using argument_type = zk::acl_version;
    using result_type   = std::size_t;

    result_type operator()(const argument_type& x) const
    {
        return zk::hash(x);
    }
};

template <>
struct hash<zk::child_version>
{
    using argument_type = zk::child_version;
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
