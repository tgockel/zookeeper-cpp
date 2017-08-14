#include "detail/subprocess.hpp"
#include "package_registry.hpp"
#include "server.hpp"

#include <iostream>

#include <signal.h>

namespace zk::server
{

server::server(std::string classpath) :
        _running(true),
        _worker([this, classpath = std::move(classpath)] () mutable { this->run_process(std::move(classpath)); })
{ }

server::~server() noexcept
{
    shutdown(true);
}

std::shared_ptr<server> server::create(package_registry& registry)
{
    // TODO: Handle the nullopt case
    return std::make_shared<server>(registry.find_newest_classpath().value());
}

void server::shutdown(bool wait_for_stop)
{
    _running = false;
    if (wait_for_stop)
        _worker.join();
}

void server::run_process(std::string classpath)
{
    detail::subprocess proc("java",
                            {
                                "-cp", std::move(classpath),
                                "org.apache.zookeeper.server.quorum.QuorumPeerMain",
                                "2181",
                                "zk-data"
                            }
                           );

    while (_running)
    {
        std::cout << proc.stdout().read();
        std::cerr << proc.stderr().read();
    }
    proc.signal(SIGTERM);
}

}
