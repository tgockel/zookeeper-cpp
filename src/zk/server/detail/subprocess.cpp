#include "subprocess.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <ostream>
#include <system_error>

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace zk::server::detail
{

static pid_t create_subproc(pipe&                     stdin_pipe,
                            pipe&                     stdout_pipe,
                            pipe&                     stderr_pipe,
                            std::string               program_name,
                            subprocess::argument_list args
                           )
{
    std::vector<const char*> arg_ptrs;
    arg_ptrs.reserve(args.size() + 2);
    arg_ptrs.emplace_back(program_name.c_str());
    std::transform(begin(args), end(args),
                   std::back_inserter(arg_ptrs),
                   [] (const std::string& s) { return s.c_str(); }
                  );
    arg_ptrs.emplace_back(nullptr);

    pid_t pid = ::fork();
    if (pid == 0) // child process
    {
        // LCOV_EXCL_START: No way to detect success at this point
        stdin_pipe.subsume_read(STDIN_FILENO, on_exec::keep_open);
        stdin_pipe.close();
        stdout_pipe.subsume_write(STDOUT_FILENO, on_exec::keep_open);
        stdout_pipe.close();
        stderr_pipe.subsume_write(STDERR_FILENO, on_exec::keep_open);
        stderr_pipe.close();

        ::execvp(program_name.c_str(),
                 const_cast<char**>(arg_ptrs.data())
                );
        // if we get here, exec failed
        std::exit(1);
        // LCOV_EXCL_STOP
    }
    else // parent process
    {
        stdin_pipe.close_read();
        stdout_pipe.close_write();
        stderr_pipe.close_write();

        return pid;
    }
}

static void dont_leak(pipe& p) noexcept
{
    auto fds = { p.native_read_handle(), p.native_write_handle() };
    for (auto fd : fds)
    {
        if (fd == -1)
            continue;

        // ignore the return code -- the inability to set FD_CLOEXEC isn't fatal, just inconvenient
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
}

subprocess::subprocess(std::string program_name, argument_list args) :
        _program_name(std::move(program_name)),
        _proc_id(-1),
        _stdin(on_exec::keep_open),
        _stdout(on_exec::keep_open),
        _stderr(on_exec::keep_open)
{
    _proc_id = create_subproc(_stdin, _stdout, _stderr, _program_name, std::move(args));

    // Set the on_exec for the pipes to no longer keep_open for future subprocesses -- there is a race condition here if
    // another thread creates a subprocess before we set FD_CLOEXEC. The worst thing that happens is we leak a couple of
    // (unused) file descriptors to the subprocess. There is no easy way to prevent this.
    dont_leak(_stdin);
    dont_leak(_stdout);
    dont_leak(_stderr);
}

subprocess::~subprocess() noexcept
{
    terminate();
}

void subprocess::terminate(duration_type time_to_abort) noexcept
{
    auto alarm_time = [&] () -> unsigned int
                      {
                          if (time_to_abort.count() <= 0)
                              return 1U;
                          else if (time_to_abort.count() > 300)
                              return 300U;
                          else
                              return static_cast<unsigned int>(time_to_abort.count());
                      }();

    for (unsigned attempt = 1U; _proc_id != -1; ++attempt)
    {
        auto old_sig_handler = ::signal(SIGALRM, [](int) { });
        signal(attempt == 1U ? SIGTERM : SIGABRT);

        int rc;
        ::alarm(alarm_time);
        if (::waitpid(_proc_id, &rc, 0) > 0)
        {
            _proc_id = -1;
        }

        ::alarm(0);
        ::signal(SIGALRM, old_sig_handler);
    }
}

bool subprocess::signal(int sig_val)
{
    if (_proc_id == -1)
        return false;

    pid_t pid = _proc_id;

    int rc = ::kill(pid, sig_val);
    if (rc == -1 && errno == ESRCH)
        return false;
    else if (rc == -1)
        throw std::system_error(errno, std::system_category());
    else
        return true;
}

}
