#pragma once

#include <zk/config.hpp>

#include <memory>
#include <string>
#include <thread>

namespace zk::server
{

class package_registry;

class server final
{
public:
    /** \warning
     *  See \c create for why this shouldn't be used.
    **/
    explicit server(std::string classpath);

    ~server() noexcept;

    /** Create a running server process with the best (newest) version from the provided \a registry.
     *
     *  \warning
     *  This interface, while convenient, is probably totally broken. Expect it to be deprecated in the not-too-distant
     *  future.
    **/
    static std::shared_ptr<server> create(package_registry& registry);

    void shutdown(bool wait_for_stop = false);

private:
    void run_process(std::string classpath);

private:
    bool               _running;
    std::thread        _worker;
};

}
