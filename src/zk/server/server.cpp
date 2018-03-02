#include "server.hpp"

#include <zk/future.hpp>

#include <cerrno>
#include <exception>
#include <iostream>
#include <system_error>

#include <signal.h>
#include <sys/select.h>

#include "classpath.hpp"
#include "configuration.hpp"
#include "detail/event_handle.hpp"
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
        _running(true),
        _shutdown_event(std::make_unique<detail::event_handle>())
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
    _running.store(false, std::memory_order_release);
    _shutdown_event->notify_one();

    if (wait_for_stop && _worker.joinable())
        _worker.join();
}

static void wait_for_event(int fd1, int fd2, int fd3)
{
    // This could be implemented with epoll instead of select, but since N=3, it doesn't really matter
    ::fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd1, &read_fds);
    FD_SET(fd2, &read_fds);
    FD_SET(fd3, &read_fds);

    int nfds = std::max(std::max(fd1, fd2), fd3) + 1;
    int rc   = ::select(nfds, &read_fds, nullptr, nullptr, nullptr);
    if (rc < 0)
    {
        if (errno == EINTR)
            return;
        else
            throw std::system_error(errno, std::system_category(), "select");
    }
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

    auto drain_pipes = [&] ()
                       {
                           bool read_anything = true;
                           while (read_anything)
                           {
                               read_anything = false;

                               auto out = proc.stdout().read();
                               if (!out.empty())
                               {
                                   read_anything = true;
                                   std::cout << out;
                               }

                               auto err = proc.stderr().read();
                               if (!err.empty())
                               {
                                   read_anything = true;
                                   std::cerr << out;
                               }
                           }
                       };

    while (_running.load(std::memory_order_acquire))
    {
        wait_for_event(proc.stdout().native_read_handle(),
                       proc.stderr().native_read_handle(),
                       _shutdown_event->native_handle()
                      );

        drain_pipes();
    }
    proc.terminate();
    drain_pipes();
}

}
