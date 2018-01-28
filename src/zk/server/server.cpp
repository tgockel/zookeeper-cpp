#include "server.hpp"

#include <zk/future.hpp>

#include <exception>
#include <iostream>

#include <signal.h>

#include "classpath.hpp"
#include "configuration.hpp"
#include "detail/subprocess.hpp"

namespace zk::server
{

static void validate_settings(const configuration& settings)
{
    if (settings.is_minimal())
    {
        return;
    }
    else if (!settings.source_file())
    {
        throw std::invalid_argument("Configuration has not been saved to a file");
    }
}

server::server(classpath packages, configuration settings) :
        _running(true)
{
    validate_settings(settings);
    _worker = std::thread([this, packages = std::move(packages), settings = std::move(settings)] ()
                          {
                              this->run_process(packages, settings);
                          }
                         );
}

server::server(configuration settings) :
        server(classpath::system_default(), std::move(settings))
{ }

server::~server() noexcept
{
    shutdown(true);
}

void server::shutdown(bool wait_for_stop)
{
    _running = false;
    if (wait_for_stop && _worker.joinable())
        _worker.join();
}

void server::run_process(const classpath& packages, const configuration& settings)
{
    detail::subprocess::argument_list args = { "-cp", packages.command_line(),
                                               "org.apache.zookeeper.server.quorum.QuorumPeerMain",
                                             };
    if (settings.is_minimal())
    {
        args.emplace_back(std::to_string(settings.client_port()));
        args.emplace_back(settings.data_directory().value());
    }
    else
    {
        args.emplace_back(settings.source_file().value());
    }

    detail::subprocess proc("java", std::move(args));

    while (_running.load(std::memory_order_relaxed))
    {
        std::cout << proc.stdout().read();
        std::cerr << proc.stderr().read();
    }
    proc.signal(SIGTERM);
}

}
