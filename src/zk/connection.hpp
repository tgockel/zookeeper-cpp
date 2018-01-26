#pragma once

#include <zk/config.hpp>

#include <chrono>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "buffer.hpp"
#include "forwards.hpp"
#include "future.hpp"
#include "string_view.hpp"

namespace zk
{

/// \addtogroup Client
/// \{

/// An actual connection to the server. The majority of methods have the same signature and meaning as \ref client.
///
/// \warning
/// A \c connection is not thread-safe -- the caller must synchronize access to it. If you are using \c client, this is
/// done for you. See the \c receive function for more information.
///
/// \see connection_zk
class connection
{
public:
    using native_handle_type = int;

    using size_type = std::size_t;

public:
    static std::shared_ptr<connection> connect(const connection_params&);

    static std::shared_ptr<connection> connect(string_view conn_string);

    virtual ~connection() noexcept;

    /** Close the connection. Realistically, this merely submits a close operation to the system. Do not assume the
     *  connection is fully closed until \c receive returns an \c event with \c state::closed.
    **/
    virtual void close() = 0;

    virtual void submit(op,       ptr<void> track) = 0;
    virtual void submit(watch_op, ptr<void> track) = 0;
    virtual void submit(multi_op, ptr<void> track) = 0;

    /// Receive at most \a max notifications from the connection and place them in \a out. If there are no notifications
    /// to receive, this function will return immediately.
    ///
    /// \param out A pointer to the location to write received notifications to. It should have at least \a max elements
    ///  to write to.
    /// \param max The maximum number of notifications to receive.
    /// \returns The number of notifications written to \a out. This will always be fewer than \a max.
    ///
    /// \note
    /// There is no logical way to have \c receive work across multiple threads. Concurrent callers would receive
    /// different sets of returned notifications. If you wish to have a multi-threaded notification delivery mechanism,
    /// you can build one using this low-level system.
    virtual size_type receive(ptr<notification> out, size_type max) noexcept;

    /// Get the native handle, which can be used to tie into event systems such as \c epoll. The handle will become
    /// readable when \c receive will return at least one notification without blocking.
    virtual native_handle_type native_handle() = 0;

    /// Get the current state of the conneciton. Most state changes should be seen through a \c notification seen
    /// through \c receive, but this can be helpful for debugging.
    virtual zk::state state() const = 0;
};

/// Used to specify parameters for a \c connection. This can either be created manually or through a
/// \ref ConnectionStrings "connection string".
class connection_params final
{
public:
    using host_list = std::vector<std::string>;

public:
    static constexpr std::chrono::milliseconds default_timeout = std::chrono::seconds(10);

public:
    /// Create an instance with default values.
    connection_params() noexcept;

    ~connection_params() noexcept;

    /// Create an instance from a connection string.
    ///
    /// \anchor ConnectionStrings
    /// \par Connection Strings
    /// Connection strings follow the standard format for URLs (`schema://host_address/path?querystring`). The `schema`
    /// is the type of connection (usually `"zk"`), `auth` is potential authentication, the `host_address` is the list
    /// of servers, the `path` is the chroot to use for this connection and the `querystring` allows for configuring
    /// advanced options. For example: `"zk://server-a:2181,server-b:2181,server-c:2181/?timeout=5"`.
    ///
    /// \par
    /// - \e schema: \ref connection_params::connection_schema
    /// - \e host_address: comma-separated list of \ref connection_params::hosts
    /// - \e path: \ref connection_params::chroot
    /// - \e querystring: Allows for an arbitrary amount of optional parameters to be specified. These are specified
    ///   with the conventional HTTP-style (e.g. `"zk://localhost/?timeout=10&read_only=true"` specifies a timeout of 10
    ///   seconds and sets a read-only client. Boolean values can be specified with \c true, \c t, or \c 1 for \c true
    ///   or \c false, \c f, or \c 0 for \c false. It is important to note that, unlike regular HTTP URLs, query
    ///   parameters which are not understood will result in an error.
    ///   - `randomize_hosts`: \ref connection_params::randomize_hosts
    ///   - `read_only`: \ref connection_params::read_only
    ///   - `timeout`: \ref connection_params::timeout
    ///
    /// \throws std::invalid_argument if the string is malformed in some way.
    static connection_params parse(string_view conn_string);

    /// \{
    /// Determines the underlying \ref zk::connection implementation to use. The valid values are \c "zk" and
    /// \c "fakezk".
    ///
    /// - `zk`: The standard-issue ZooKeeper connection to a real ZooKeeper cluster. This schema uses
    ///   \ref zk::connection_zk as the underlying connection.
    /// - `fakezk`: Create a client connected to a fake ZooKeeper server (\ref zk::fake::server). Here, the
    ///   `host_address` refers to the name of the in-memory DB created when the server instance was. This schema uses
    ///   \ref zk::fake::connection_fake as the underlying connection.
    const std::string& connection_schema() const { return _connection_schema; }
    std::string&       connection_schema()       { return _connection_schema; }
    /// \}

    /// \{
    /// Addresses for the ensemble to connect to. This can be IP addresses (IPv4 or IPv6) or hostnames, but IP
    /// addresses are the recommended method of specification. If the port is left unspecified, \c 2181 is assumed (as
    /// it is the de facto standard for ZooKeeper server). For IPv6 addresses, use the boxed format (e.g. `[::1]:2181`);
    /// this is required even when the port is \c 2181 to disambiguate between a host named in hexadecimal and an IPv6
    /// address (e.g. `"[fd2d:8413:d6c6::73b]"` or `"[::1]:2181"`).
    const host_list& hosts() const { return _hosts; }
    host_list&       hosts()       { return _hosts; }
    /// \}

    /// \{
    /// Specifying a value for \c chroot as something aside from \c "" or \c "/" will run the client commands while
    /// interpreting all paths relative to the specified path. For example, specifying `"/app/a"` will make requests for
    /// `"/foo/bar"` sent to `"/app/a/foo/bar"` (from the server's perspective). If unspecified, the path will be
    /// treated as `"/"`.
    const std::string& chroot() const { return _chroot; }
    std::string&       chroot()       { return _chroot; }
    /// \}

    /// \{
    /// Connect to a host at random (as opposed to attempting connections in order)? The default is to randomize (the
    /// use cases for sequential connections are usually limited to testing purposes).
    bool  randomize_hosts() const { return _randomize_hosts; }
    bool& randomize_hosts()       { return _randomize_hosts; }
    /// \}

    /// \{
    /// Allow connections to read-only servers? The default (\c false) is to disallow. **/
    bool  read_only() const { return _read_only; }
    bool& read_only()       { return _read_only; }
    /// \}

    /// \{
    /// The session timeout between this client and the server. The server will attempt to respect this value, but will
    /// automatically use a lower timeout value if this value is too large (see the ZooKeeper Programmer's Guide for
    /// more information on maximum values). The default is 10 seconds.
    ///
    /// When specified in a query string, this value is specified either with floating-point seconds or as an ISO 8601
    /// duration (`PT8.93S` for \c 8.930 seconds).
    std::chrono::milliseconds  timeout() const { return _timeout; }
    std::chrono::milliseconds& timeout()       { return _timeout; }
    /// \}

private:
    std::string               _connection_schema;
    host_list                 _hosts;
    std::string               _chroot;
    bool                      _randomize_hosts;
    bool                      _read_only;
    std::chrono::milliseconds _timeout;
};

bool operator==(const connection_params& lhs, const connection_params& rhs);
bool operator!=(const connection_params& lhs, const connection_params& rhs);

std::string to_string(const connection_params&);
std::ostream& operator<<(std::ostream&, const connection_params&);

/// \}

}
