#pragma once

#include <zk/config.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "configuration.hpp"

namespace zk::server
{

/** \addtogroup Server
 *  \{
**/

class classpath;
class server;

/// Create and manage a group of \ref server instances on this local machine (most likely in a single ensemble). This is
/// exclusively for testing, as running multiple peers on a single machine is a very bad idea in production.
///
/// \code
/// auto servers = zk::server::server_group::make_ensemble(3U,
///                                                        zk::server::configuration::make_minimal("test-data")
///                                                       );
/// servers.start_all_servers();
/// auto client = zk::client::connect(servers.get_connection_string()).get();
/// // do things with client...
/// \endcode
class server_group final
{
public:
    /// Create an empty server group.
    server_group() noexcept;

    /// Move-construct a server group.
    server_group(server_group&&) noexcept;

    /// Move-assign a server group.
    server_group& operator=(server_group&&) noexcept;

    /// Create an ensemble of the given \a size. None of the servers will be started.
    ///
    /// \param size The size of the ensemble. This should be an odd number.
    /// \param base_settings The basic settings to use for every server.
    static server_group make_ensemble(std::size_t size, const configuration& base_settings);

    /// Get a connection string which can connect to any the servers in the group.
    const std::string& get_connection_string();

    /// Start all servers in the group. Note that this does not wait for the servers to be up-and-running. Use a
    /// \ref client instance to check for connectability.
    void start_all_servers(const classpath& packages);

    /// How many servers are in this group?
    std::size_t size() const { return _servers.size(); }

private:
    struct info;

    using server_map_type = std::map<server_id, std::shared_ptr<info>>;

private:
    server_map_type _servers;
    std::string     _conn_string;
};

/// \}

}
