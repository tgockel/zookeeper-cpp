#pragma once

#include <zk/config.hpp>

#include <chrono>
#include <iosfwd>
#include <memory>
#include <mutex>
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
/// \see connection_zk
class connection
{
public:
    static std::shared_ptr<connection> connect(const connection_params&);

    static std::shared_ptr<connection> connect(string_view conn_string);

    virtual ~connection() noexcept;

    virtual void close() = 0;

    virtual future<get_result> get(string_view path) = 0;

    virtual future<watch_result> watch(string_view path) = 0;

    virtual future<get_children_result> get_children(string_view path) = 0;

    virtual future<watch_children_result> watch_children(string_view path) = 0;

    virtual future<exists_result> exists(string_view path) = 0;

    virtual future<watch_exists_result> watch_exists(string_view path) = 0;

    virtual future<create_result> create(string_view   path,
                                         const buffer& data,
                                         const acl&    rules,
                                         create_mode   mode
                                        ) = 0;

    virtual future<set_result> set(string_view path, const buffer& data, version check) = 0;

    virtual future<void> erase(string_view path, version check) = 0;

    virtual future<get_acl_result> get_acl(string_view path) const = 0;

    virtual future<void> set_acl(string_view path, const acl& rules, acl_version check) = 0;

    virtual future<multi_result> commit(multi_op&& txn) = 0;

    virtual future<void> load_fence() = 0;

    virtual zk::state state() const = 0;

    /// Watch for a state change.
    virtual future<zk::state> watch_state();

protected:
    /// Call this from derived classes when a session event happens. This triggers the delivery of all promises of state
    /// changes (issued through \ref watch_state).
    virtual void on_session_event(zk::state new_state);

private:
    mutable std::mutex              _state_change_promises_protect;
    std::vector<promise<zk::state>> _state_change_promises;
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
