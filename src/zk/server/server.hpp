#pragma once

#include <zk/config.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <string>
#include <thread>

namespace zk::server
{

namespace detail
{

class event_handle;

}

/// \defgroup Server
/// Control a ZooKeeper \ref server process.
/// \{

class classpath;
class configuration;

/// Controls a ZooKeeper server process on this local machine.
class server final
{
public:
    /// Create a running server process with the specified \a packages and \a settings.
    ///
    /// \param packages The classpath to use to find ZooKeeper's \c QuorumPeerMain class.
    /// \param settings The server settings to run with.
    /// \throws std::invalid_argument If `settings.is_minimal()` is \c false and `settings.source_file()` is \c nullopt.
    ///  This is because non-minimal configurations require ZooKeeper to be launched with a file.
    explicit server(classpath packages, configuration settings);

    /// Create a running server with the specified \a settings using the system-provided default packages for ZooKeeper
    /// (see \ref classpath::system_default).
    ///
    /// \param settings The server settings to run with.
    /// \throws std::invalid_argument If `settings.is_minimal()` is \c false and `settings.source_file()` is \c nullopt.
    ///  This is because non-minimal configurations require ZooKeeper to be launched with a file.
    explicit server(configuration settings);

    server(const server&) = delete;

    ~server() noexcept;

    /// Initiate shutting down the server process. For most usage, this is not needed, as it is called automatically
    /// from the destructor.
    ///
    /// \param wait_for_stop If \c true, wait for the process to run until termination instead of simply initiating the
    ///  termination.
    void shutdown(bool wait_for_stop = false);

private:
    void run_process(const classpath&, const configuration&);

private:
    std::atomic<bool>                     _running;
    std::unique_ptr<detail::event_handle> _shutdown_event;
    std::thread                           _worker;

    // NOTE: The configuration is NOT stored in the server object. This is because configuration can be changed by the
    // ZK process in cases like ensemble reconfiguration. It is the job of run_process to deal with this.
};

/// \}

}
