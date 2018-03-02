#pragma once

#include <zk/config.hpp>

#include <chrono>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "pipe.hpp"

namespace zk::server::detail
{

/// Represents an owned subprocess.
class subprocess
{
public:
    using handle        = int;
    using argument_list = std::vector<std::string>;
    using duration_type = std::chrono::seconds;

public:
    explicit subprocess(std::string program_name, argument_list args = argument_list());

    subprocess(const subprocess&) = delete;
    subprocess& operator=(const subprocess&) = delete;

    ~subprocess() noexcept;

    /// Send a signal to this subprocess.
    ///
    /// \returns \c true if the signal likely reached the subprocess; \c false if it might not have (this can happen if
    ///  the subprocess has already terminated).
    bool signal(int sig_val);

    pipe& stdin()  noexcept { return _stdin; }
    pipe& stdout() noexcept { return _stdout; }
    pipe& stderr() noexcept { return _stderr; }

    /// Terminate the process if it is still running. In the first attempt to terminate, \c SIGTERM is used. If the
    /// process has not terminated before \a time_to_abort has passed, the process is signalled again with \c SIGABRT.
    void terminate(duration_type time_to_abort = std::chrono::seconds(1U)) noexcept;

private:
    std::string _program_name;
    handle      _proc_id;
    pipe        _stdin;
    pipe        _stdout;
    pipe        _stderr;
};

}
