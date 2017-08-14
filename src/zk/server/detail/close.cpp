#include "close.hpp"

#include <string>
#include <system_error>

#include <cerrno>
#include <unistd.h>

namespace zk::server::detail
{

void close(int fd)
{
    if (::close(fd) == -1)
    {
        // LCOV_EXCL_START: Close will always work -- push failures to the consumer
        switch (errno)
        {
            case EINTR: return; // POSIX guarantees we're still closed in EINTR cases
            case EIO:   throw std::system_error(errno, std::system_category(), "I/O error on close()");
            default:    throw std::system_error(errno, std::system_category(), "System error on close()");
        }
        // LCOV_EXCL_STOP
    }
}

}
