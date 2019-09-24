#pragma once

#include <zk/config.hpp>
#include "exceptions.hpp"

#include <iosfwd>
#include <string>
#include <system_error>

namespace zk
{

/// \addtogroup Client
/// \{

/// Code for all \ref error types thrown by the client library.
///
/// \see error
enum class error_code : int
{
    ok                          =    0, //!< Never thrown.
    connection_loss             =   -4, //!< Code for \ref connection_loss.
    marshalling_error           =   -5, //!< Code for \ref marshalling_error.
    not_implemented             =   -6, //!< Code for \ref not_implemented.
    invalid_arguments           =   -8, //!< Code for \ref invalid_arguments.
    new_configuration_no_quorum =  -13, //!< Code for \ref new_configuration_no_quorum.
    reconfiguration_in_progress =  -14, //!< Code for \ref reconfiguration_in_progress.
    no_entry                    = -101, //!< Code for \ref no_entry.
    not_authorized              = -102, //!< Code for \ref not_authorized.
    version_mismatch            = -103, //!< Code for \ref version_mismatch.
    no_children_for_ephemerals  = -108, //!< Code for \ref no_children_for_ephemerals.
    entry_exists                = -110, //!< Code for \ref entry_exists.
    not_empty                   = -111, //!< Code for \ref not_empty.
    session_expired             = -112, //!< Code for \ref session_expired.
    authentication_failed       = -115, //!< Code for \ref authentication_failed.
    closed                      = -116, //!< Code for \ref closed.
    read_only_connection        = -119, //!< Code for \ref read_only_connection.
    ephemeral_on_local_session  = -120, //!< Code for \ref ephemeral_on_local_session.
    reconfiguration_disabled    = -123, //!< Code for \ref reconfiguration_disabled.
    transaction_failed          = -199, //!< Code for \ref transaction_failed.
};

/// Check if the provided \a code is an exception code for a \ref transport_error type of exception.
inline constexpr bool is_transport_error(error_code code)
{
    return code == error_code::connection_loss
        || code == error_code::marshalling_error;
}

/// Check if the provided \a code is an exception code for a \ref invalid_arguments type of exception.
inline constexpr bool is_invalid_arguments(error_code code)
{
    return code == error_code::invalid_arguments
        || code == error_code::authentication_failed;
}

/// Check if the provided \a code is an exception code for a \ref invalid_ensemble_state type of exception.
inline constexpr bool is_invalid_ensemble_state(error_code code)
{
    return code == error_code::new_configuration_no_quorum
        || code == error_code::reconfiguration_disabled
        || code == error_code::reconfiguration_in_progress;
}

/// Check if the provided \a code is an exception code for a \ref invalid_connection_state type of exception.
inline constexpr bool is_invalid_connection_state(error_code code)
{
    return code == error_code::closed
        || code == error_code::ephemeral_on_local_session
        || code == error_code::not_authorized
        || code == error_code::read_only_connection
        || code == error_code::session_expired;
}

/// Check if the provided \a code is an exception code for a \ref check_failed type of exception.
inline constexpr bool is_check_failed(error_code code)
{
    return code == error_code::no_children_for_ephemerals
        || code == error_code::no_entry
        || code == error_code::entry_exists
        || code == error_code::not_empty
        || code == error_code::transaction_failed
        || code == error_code::version_mismatch;
}

std::ostream& operator<<(std::ostream&, const error_code&);

std::string to_string(const error_code&);

/// Throw an exception for the given \a code. This will use the proper refined exception type (such as \ref no_entry) if
/// one exists.
[[noreturn]]
void throw_error(error_code code);

/// Get an \c zk::exception_ptr containing an exception with the proper type for the given \a code.
///
/// \see throw_error
zk::exception_ptr get_exception_ptr_of(error_code code);

/// Get the \c std::error_category capable of describing ZooKeeper-provided error codes.
///
/// \see error
const std::error_category& error_category();

/// Base error type for all errors raised by this library.
///
/// \see error_code
class error :
        public std::system_error
{
public:
    explicit error(error_code code, const std::string& description);

    virtual ~error() noexcept;

    /// The code representation of this error.
    error_code code() const { return _code; }

private:
    error_code _code;
};

/// Base types for errors that occurred while transporting data across a network.
///
/// \see is_transport_error
class transport_error :
        public error
{
public:
    explicit transport_error(error_code code, const std::string& description);

    virtual ~transport_error() noexcept;
};

/// Connection to the server has been lost before the attempted operation was verified as completed.
///
/// When thrown on an attempt to perform a modification, it is important to remember that it is possible to see this
/// error and have the operation be a success. For example, a \ref client::set operation can complete on the server, but
/// the client can experience a \ref connection_loss before the server replies with OK.
///
/// \see session_expired
class connection_loss:
        public transport_error
{
public:
    explicit connection_loss();

    virtual ~connection_loss() noexcept;
};

/// An error occurred while marshalling data. The most common cause of this is exceeding the Jute buffer size -- meaning
/// the transaction was too large (check the server logs for messages containing `"Unreasonable length"`). If that is
/// the case, the solution is to change `jute.maxbuffer` on all servers (see the
/// <a href="https://zookeeper.apache.org/doc/r3.4.10/zookeeperAdmin.html">ZooKeeper Administrator's Guide</a> for more
/// information and a stern warning). Another possible cause is the system running out of memory, but due to overcommit,
/// OOM issues rarely manifest so cleanly.
class marshalling_error:
        public transport_error
{
public:
    explicit marshalling_error();

    virtual ~marshalling_error() noexcept;
};

/// Operation was attempted that was not implemented. If you happen to be writing a \ref connection implementation, you
/// are encouraged to raise this error in cases where you have not implemented an operation.
class not_implemented:
        public error
{
public:
    /// \param op_name the name of the attempted operation.
    explicit not_implemented(ptr<const char> op_name);

    virtual ~not_implemented() noexcept;
};

/// Arguments to an operation were invalid.
class invalid_arguments :
        public error
{
public:
    explicit invalid_arguments(error_code code, const std::string& description);

    explicit invalid_arguments();

    virtual ~invalid_arguments() noexcept;
};

/// The server rejected the connection due to invalid authentication information. Depending on the authentication
/// schemes enabled on the server, the authentication information sent might be explicit (in the case of the \c "digest"
/// scheme) or implicit (in the case of the \c "ip" scheme). The connection must be recreated with the proper
/// credentials to function.
///
/// \see acl_rule
class authentication_failed:
        public invalid_arguments
{
public:
    explicit authentication_failed();

    virtual ~authentication_failed() noexcept;
};

/// Base exception for cases where the ensemble is in an invalid state to perform a given action. While errors such as
/// \ref connection_loss might also imply a bad ensemble (no quorum means no connection is possible), these errors are
/// explicit rejections from the server.
class invalid_ensemble_state :
        public error
{
public:
    explicit invalid_ensemble_state(error_code code, const std::string& description);

    virtual ~invalid_ensemble_state() noexcept;
};

/// Raised when attempting an ensemble reconfiguration, but the proposed new ensemble would not be able to form quorum.
/// This happens when not enough time has passed for potential new servers to sync with the leader. If the proposed new
/// ensemble is up and running, the solution is usually to simply wait longer and attempt reconfiguration later.
class new_configuration_no_quorum:
        public invalid_ensemble_state
{
public:
    explicit new_configuration_no_quorum();

    virtual ~new_configuration_no_quorum() noexcept;
};

/// An attempt was made to reconfigure the ensemble, but there is already a reconfiguration in progress. Concurrent
/// reconfiguration is not supported.
class reconfiguration_in_progress:
        public invalid_ensemble_state
{
public:
    explicit reconfiguration_in_progress();

    virtual ~reconfiguration_in_progress() noexcept;
};

/// The ensemble does not support reconfiguration.
class reconfiguration_disabled:
        public invalid_ensemble_state
{
public:
    explicit reconfiguration_disabled();

    virtual ~reconfiguration_disabled() noexcept;
};

/// Base type for errors generated because the connection is misconfigured.
class invalid_connection_state :
        public error
{
public:
    explicit invalid_connection_state(error_code code, const std::string& description);

    virtual ~invalid_connection_state() noexcept;
};

/// The client session has been ended by the server. When this occurs, all ephemerals associated with the session are
/// deleted and standing watches are cancelled.
///
/// This error is somewhat easy to confuse with \ref connection_loss, as they commonly happen around the same time. The
/// key difference is a \ref session_expired is an explicit error delivered from the server, whereas
/// \ref connection_loss is a client-related notification. A \ref connection_loss is \e usually followed by
/// \ref session_expired, but this is not guaranteed. If the client reconnects to a different server before the quorum
/// removes the session, the connection can move back to \ref state::connected without losing the session. The mechanism
/// of resuming a session can happen even in cases of quorum loss, as session expiration requires a leader in order to
/// proceed, so a client reconnecting soon enough after the ensemble forms quorum and elects a leader will resume the
/// session, even if the quorum has been lost for days.
class session_expired:
        public invalid_connection_state
{
public:
    explicit session_expired();

    virtual ~session_expired() noexcept;
};

/// An attempt was made to read or write to a ZNode when the connection does not have permission to do.
class not_authorized:
        public invalid_connection_state
{
public:
    explicit not_authorized();

    virtual ~not_authorized() noexcept;
};

/// The connection is closed. This exception is delivered for all unfilled operations and watches when the connection is
/// closing.
class closed:
        public invalid_connection_state
{
public:
    explicit closed();

    virtual ~closed() noexcept;
};

/// An attempt was made to create an ephemeral entry, but the connection has a local session.
///
/// \see connection_params::local
class ephemeral_on_local_session:
        public invalid_connection_state
{
public:
    explicit ephemeral_on_local_session();

    virtual ~ephemeral_on_local_session() noexcept;
};

/// A write operation was attempted on a read-only connection.
///
/// \see connection_params::read_only
class read_only_connection :
        public invalid_connection_state
{
public:
    explicit read_only_connection();

    virtual ~read_only_connection() noexcept;
};

/// Base exception for cases where a write operation was rolled back due to a failed check. There are more details in
/// derived types such as \ref no_entry or \ref bad_version.
class check_failed :
        public error
{
public:
    explicit check_failed(error_code code, const std::string& description);

    virtual ~check_failed() noexcept;
};

/// Thrown from read operations when attempting to read a ZNode that does not exist.
class no_entry :
        public check_failed
{
public:
    explicit no_entry();

    virtual ~no_entry() noexcept;
};

/// Thrown when attempting to create a ZNode, but one already exists at the specified path.
class entry_exists :
        public check_failed
{
public:
    explicit entry_exists();

    virtual ~entry_exists() noexcept;
};

/// Thrown when attempting to erase a ZNode that has children.
class not_empty :
        public check_failed
{
public:
    explicit not_empty();

    virtual ~not_empty() noexcept;
};

/// Thrown from modification operations when a version check is specified and the value in the database does not match
/// the expected.
class version_mismatch :
        public check_failed
{
public:
    explicit version_mismatch();

    virtual ~version_mismatch() noexcept;
};

/// Ephemeral ZNodes cannot have children.
class no_children_for_ephemerals :
        public check_failed
{
public:
    explicit no_children_for_ephemerals();

    virtual ~no_children_for_ephemerals() noexcept;
};

/// Thrown from \ref client::commit when a transaction cannot be committed to the system. Check the
/// \ref underlying_cause to see the specific error and \ref failed_op_index to see what operation failed.
class transaction_failed :
        public check_failed
{
public:
    explicit transaction_failed(error_code code, std::size_t op_index);

    virtual ~transaction_failed() noexcept;

    /// The underlying cause that caused this transaction to be aborted. For example, if a \ref op::set operation is
    /// attempted on a node that does not exist, this will be \ref error_code::no_entry.
    error_code underlying_cause() const { return _underlying_cause; }

    /// The transaction index which caused the error (0 indexed). If the 3rd operation in the \ref multi_op could not be
    /// committed, this will be 2.
    std::size_t failed_op_index() const { return _op_index; }

private:
    error_code  _underlying_cause;
    std::size_t _op_index;
};

/// \}

}

namespace std
{

template <>
struct is_error_code_enum<zk::error_code> :
        true_type
{
};

}
