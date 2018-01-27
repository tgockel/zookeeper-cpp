#include "server.hpp"

#include <zk/future.hpp>

#include <exception>
#include <iostream>

#include <signal.h>

#include "configuration.hpp"
#include "detail/subprocess.hpp"
#include "package_registry.hpp"

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

server::server(std::string classpath, configuration settings) :
        _running(true)
{
    validate_settings(settings);
    _worker = std::thread([this,
                           classpath    = std::move(classpath),
                           settings     = std::move(settings)
                          ] () mutable
                          {
                              this->run_process(std::move(classpath), std::move(settings));
                          }
                         );
}

server::~server() noexcept
{
    shutdown(true);
}

std::shared_ptr<server> server::create(package_registry& registry, configuration settings)
{
    // TODO: Handle the nullopt case
    return std::make_shared<server>(registry.find_newest_classpath().value(), std::move(settings));
}

void server::shutdown(bool wait_for_stop)
{
    _running = false;
    if (wait_for_stop && _worker.joinable())
        _worker.join();
}

void server::run_process(std::string classpath, configuration&& settings)
{
    detail::subprocess::argument_list args = { "-cp", std::move(classpath),
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
