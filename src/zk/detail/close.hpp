#pragma once

#include <zk/config.hpp>

namespace zk::detail
{

/** A safe wrapper around the Linux \c close call. In particular, it transforms certain error codes that matter into
 *  exceptions and happily ignores error codes that still close the file descriptor (\c EINTR).
 *
 *  \throws system_error if an I/O error occurs (this can happen if the kernel defers writes -- although in general, you
 *   have made a grave error by not performing a \c sync before \c close).
 *  \throws system_error if \a fd is not a file descriptor.
**/
void close(int fd);

}
