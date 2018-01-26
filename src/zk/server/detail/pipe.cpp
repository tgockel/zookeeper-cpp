#include "pipe.hpp"

#include <zk/detail/close.hpp>

#include <algorithm>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>

namespace zk::server::detail
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pipe_closed                                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pipe_closed::pipe_closed() :
        std::runtime_error("I/O operation on closed pipe")
{ }

pipe_closed::~pipe_closed() noexcept
{ }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pipe                                                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int on_exec_flags(on_exec exec)
{
    switch (exec)
    {
        case on_exec::close:
            return O_CLOEXEC;
        case on_exec::keep_open:
        default:
            return 0;
    }
}

pipe::pipe(on_exec exec) :
        _read_fd(-1),
        _write_fd(-1)
{
    int fds[2];
    if (::pipe2(fds, O_NONBLOCK | on_exec_flags(exec)))
        throw std::system_error(errno, std::system_category(), "Could not create pipe");

    _read_fd = fds[0];
    _write_fd = fds[1];
}

pipe::~pipe() noexcept
{
    close();
}

static void close_if_open(int& fd)
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

void pipe::close()
{
    close_write();
    close_read();
}

void pipe::close_read()
{
    close_if_open(_read_fd);
}

void pipe::close_write()
{
    close_if_open(_write_fd);
}

std::string pipe::read(optional<std::size_t> max)
{
    if (_read_fd == -1)
        throw pipe_closed();

    std::string out;
    if (max)
        out.reserve(max.value());

    while (true)
    {
        char read_buf[4096];

        ssize_t rc = ::read(_read_fd, read_buf, sizeof read_buf);
        if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            rc = 0;

        if (rc < 0 && errno == EINTR)
        {
            continue;
        }
        else if (rc < 0 && errno != EINTR)
        {
            if (out.empty())
                throw std::system_error(errno, std::system_category(), "Failed to read from pipe");
            else
                return out;
        }
        else if (rc == 0)
        {
            return out;
        }
        else
        {
            out.append(read_buf, rc);
        }
    }
}

void pipe::write(const std::string& contents)
{
    if (_write_fd == -1)
        throw pipe_closed();

    ptr<const char> write_ptr = contents.data();
    while (write_ptr < contents.data() + contents.size())
    {
        ssize_t goal_dist = std::distance(write_ptr, contents.data() + contents.size());
        ssize_t rc = ::write(_write_fd, write_ptr, goal_dist);
        if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            rc = 0;

        if (rc < 0 && errno == EINTR)
        {
            continue;
        }
        else if (rc < 0 && errno == EPIPE)
        {
            throw pipe_closed();
        }
        else if (rc < 0)
        {
            throw std::system_error(errno, std::system_category(), "Failed to write to pipe");
        }
        else
        {
            write_ptr += rc;
        }
    }
}

static int dup3(int src_fd, int redir_fd, on_exec exec)
{
    while (true)
    {
        int rc = ::dup3(src_fd, redir_fd, on_exec_flags(exec));
        if (rc == -1 && errno == EINTR)
            continue;
        else if (rc == -1 && errno != EINTR)
            throw std::system_error(errno, std::system_category());
        else
            return rc;
    }
}

void pipe::subsume_read(handle fd, on_exec exec)
{
    dup3(_read_fd, fd, exec);
    // close now unused read side of this pipe
    close_read();
}

void pipe::subsume_write(handle fd, on_exec exec)
{
    dup3(_write_fd, fd, exec);
    close_write();
}

pipe::handle pipe::native_read_handle()
{
    return _read_fd;
}

pipe::handle pipe::native_write_handle()
{
    return _write_fd;
}

}
