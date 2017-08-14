#pragma once

#include <zk/config.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "pipe.hpp"

namespace zk::server::detail
{

/** Represents an owned subprocess. **/
class subprocess
{
public:
    using handle = int;
    using argument_list = std::vector<std::string>;

public:
    explicit subprocess(std::string program_name, argument_list args = argument_list());

    subprocess(const subprocess&) = delete;
    subprocess& operator=(const subprocess&) = delete;

    ~subprocess() noexcept;

    /** Send a signal to this subprocess.
     *
     *  \param whole_group Should the entire process group be signalled or just the root process?
     *  \returns \c true if the signal likely reached the subprocess; \c false if it might not have (this can happen if
     *   the subprocess has already terminated).
    **/
    bool signal(int sig_val, bool whole_group = false);

    pipe& stdin()  noexcept { return _stdin; }
    pipe& stdout() noexcept { return _stdout; }
    pipe& stderr() noexcept { return _stderr; }

private:
    std::string _program_name;
    handle      _proc_id;
    pipe        _stdin;
    pipe        _stdout;
    pipe        _stderr;
};

}
