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

/** Create and manage a group of \c server instances on this local machine (most likely in a single ensemble). This is
 *  exclusively for testing, as running multiple peers on a single machine is a very bad idea in production.
**/
class server_group final
{
public:
    server_group() noexcept;

    server_group(server_group&&) noexcept;

    server_group& operator=(server_group&&) noexcept;

    /** Create an ensemble of the given \a size. None of the servers will be started.
     *
     *  \param size The size of the ensemble. This should be an odd number.
     *  \param base_settings The basic settings to use for every server.
    **/
    static server_group make_ensemble(std::size_t size, const configuration& base_settings);

    /** Get a connection string which can connect to any the servers in the group. **/
    const std::string& get_connection_string();

    /** Start all servers in the group. **/
    void start_all_servers(const classpath& packages);

    std::size_t size() const { return _servers.size(); }

private:
    struct info;

    using server_map_type = std::map<server_id, std::shared_ptr<info>>;

private:
    server_map_type _servers;
    std::string     _conn_string;
};

/** \} **/

}
