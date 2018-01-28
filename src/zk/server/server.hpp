#pragma once

#include <zk/config.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <string>
#include <thread>

namespace zk::server
{

/** \defgroup Server
 *  Control a ZooKeeper \ref server process.
 *  \{
**/

class configuration;
class package_registry;

/** Controls a ZooKeeper server process on this local machine. **/
class server final
{
public:
    /** \warning
     *  See \c create for why this shouldn't be used.
    **/
    explicit server(std::string classpath, configuration settings);

    server(const server&) = delete;

    ~server() noexcept;

    /** Create a running server process with the best (newest) version from the provided \a registry.
     *
     *  \warning
     *  This interface, while convenient, is probably totally broken. Expect it to be deprecated in the not-too-distant
     *  future.
    **/
    static std::shared_ptr<server> create(const package_registry& registry, configuration settings);

    void shutdown(bool wait_for_stop = false);

private:
    void run_process(std::string, configuration&&);

private:
    std::atomic<bool> _running;
    std::thread       _worker;

    // NOTE: The configuration is NOT stored in the server object. This is because configuration can be changed by the
    // ZK process in cases like ensemble reconfiguration. It is the job of run_process to deal with this.
};

/** \} **/

}
