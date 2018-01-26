#include "event_handle.hpp"
#include "close.hpp"

#include <cerrno>
#include <cstdint>
#include <system_error>

#include <sys/eventfd.h>
#include <unistd.h>

namespace zk::detail
{

event_handle::event_handle() :
        _fd(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))
{ }

event_handle::~event_handle() noexcept
{
    close();
}

void event_handle::close() noexcept
{
    if (_fd != -1)
    {
        close(_fd);
        _fd = -1;
    }
}

void event_handle::notify_one()
{
    std::uint64_t x = 1;
    if (::write(_fd, &x, sizeof x) == -1 && errno != EAGAIN)
        throw std::system_error(errno, std::system_category(), "event_handle::notify_one()");
}

bool event_handle::try_wait()
{
    std::uint64_t burn;
    if (::read(_fd, &burn, sizeof burn) == -1)
        return errno == EAGAIN ? false
                               : throw std::system_error(errno, std::system_category(), "event_handle::try_wait()");
    else
        return true;
}

}
