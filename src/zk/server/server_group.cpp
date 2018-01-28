#include "server_group.hpp"

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <sys/stat.h>
#include <sys/types.h>

#include "server.hpp"
#include "configuration.hpp"

namespace zk::server
{

struct server_group::info
{
    configuration           settings;
    std::string             name;
    std::string             path;
    std::uint16_t           peer_port;
    std::uint16_t           leader_port;
    std::shared_ptr<server> instance;

    info(const configuration& base) :
            settings(base)
    { }
};

server_group::server_group() noexcept = default;

server_group::server_group(server_group&&) noexcept = default;

server_group& server_group::operator=(server_group&&) noexcept = default;

static void create_directory(const std::string& path)
{
    if (::mkdir(path.c_str(), 0755))
    {
        throw std::system_error(errno, std::system_category());
    }
}

static void save_id_file(const std::string& path, const std::string& id)
{
    std::ofstream ofs(path.c_str());
    if (!ofs)
        throw std::runtime_error("IO failure");
    ofs << id << '\n';
    ofs.flush();
}

server_group server_group::make_ensemble(std::size_t size, const configuration& base_settings_in)
{
    auto base_settings = base_settings_in;

    if (!base_settings.data_directory())
        throw std::invalid_argument("Settings must specify a base directory");

    auto base_directory = base_settings.data_directory().value();
    auto base_port      = base_settings.client_port() == configuration::default_client_port ? std::uint16_t(18500)
                                                                                            : base_settings.client_port();

    server_group out;
    for (std::size_t idx = 0U; idx < size; ++idx)
    {
        auto px    = std::make_shared<info>(base_settings);
        auto& x    = *px;
        x.name     = std::to_string(idx + 1);
        x.path     = base_directory + "/" + x.name;
        x.settings
            .client_port(base_port++)
            .data_directory(x.path + "/data")
            ;
        x.peer_port   = base_port++;
        x.leader_port = base_port++;

        out._servers.emplace(x.name, px);
    }

    std::ostringstream conn_str_os;
    conn_str_os << "zk://";
    bool first = true;

    create_directory(base_directory);
    for (auto& [name, srvr] : out._servers)
    {
        for (const auto& [name2, srvr2] : out._servers)
        {
            srvr->settings.add_server(name2, "127.0.0.1", srvr2->peer_port, srvr2->leader_port);
        }

        create_directory(srvr->path);
        create_directory(srvr->path + "/data");
        save_id_file(srvr->path + "/data/myid", name);
        srvr->settings.save_file(srvr->path + "/settings.cfg");

        if (!std::exchange(first, false))
            conn_str_os << ',';
        conn_str_os << "127.0.0.1:" << srvr->settings.client_port();
    }
    conn_str_os << '/';
    out._conn_string = conn_str_os.str();

    return out;
}

const std::string& server_group::get_connection_string()
{
    return _conn_string;
}

void server_group::start_all_servers(const package_registry& packages)
{
    for (auto& [name, srvr] : _servers)
    {
        static_cast<void>(name);

        if (!srvr->instance)
        {
            srvr->instance = server::create(packages, srvr->settings);
        }
    }
}

}
