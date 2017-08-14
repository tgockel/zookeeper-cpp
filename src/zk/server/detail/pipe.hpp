#pragma once

#include <zk/config.hpp>
#include <zk/optional.hpp>

#include <stdexcept>
#include <string>

namespace zk::server::detail
{

/** Used to specify behavior of POSIX resources when \c exec is called. **/
enum class on_exec
{
    /** Enable the close-on-exec flag for an opened handle. This means that any subprocess created with \c exec will not
     *  be able to access the resource of the parent process.
    **/
    close,
    /** Keep the file descriptor open for any opened subprocesses. You probably do not want to use this flag unless you
     *  intend to share a pipe with the subprocess. In general, it is safer for the subprocess to open the same file
     *  itself.
    **/
    keep_open,
};

/** Thrown when a read or write operation is attempted on a \c pipe which is closed. **/
class pipe_closed :
        public std::runtime_error
{
public:
    pipe_closed();

    virtual ~pipe_closed() noexcept;
};

/** A unidirectional data channel that can be used for interprocess communication or as a signal-safe mechanism for
 *  in-process communication.
**/
class pipe final
{
public:
    using handle = int;

public:
    /** Create a pipe.
     *
     *  \param exec Should this pipe stay open when \c exec is called? See \c on_exec for more details.
     *  \throws std::system_error if the pipe could not be created.
    **/
    explicit pipe(on_exec exec = on_exec::close);

    pipe(const pipe&) = delete;
    pipe& operator=(const pipe&) = delete;

    ~pipe() noexcept;

    /** Close the read and write ends of this pipe. It is safe to call this multiple times (subsequent calls to \c close
     *  will have no effect).
    **/
    void close();

    /** Similar to \c close, but only close the read end of the pipe. **/
    void close_read();

    /** Similar to \c close, but only close the write end of the pipe. **/
    void close_write();

    /** Read from the pipe (up to \a max). If there is nothing to read, an empty string is returned.
     *
     *  \param max The maximum amount of bytes to read in a single pass. If unspecified, this will read until the pipe
     *   appears to be empty.
     *  \throws pipe_closed if the pipe is already closed.
    **/
    std::string read(optional<std::size_t> max = nullopt);

    /** Write the \a contents into the pipe.
     *
     *  \throws pipe_closed if the pipe is already closed (this typically happens when communicating with a subprocess
     *   which has been terminated).
    **/
    void write(const std::string& contents);

    /** Redirect the provided \a fd to read from this pipe instead of what it was originally reading from.
     *
     *  \example
     *  This redirects standard input to be controlled by a pipe. See \c subprocess for the use case.
     *  \code
     *  pipe p;
     *  p.subsume_read(STDIN_FILENO);
     *  // Now p.write(...) calls will be picked up by reads to standard input
     *  \endcode
    **/
    void subsume_read(handle fd, on_exec exec = on_exec::close);

    /** Redirect the provided \a fd to write to this pipe instead of what it was originally writing to. **/
    void subsume_write(handle fd, on_exec exec = on_exec::close);

    /** Get the native handle for the read end of this pipe. **/
    handle native_read_handle();

    /** Get the native handle for the write end of this pipe. **/
    handle native_write_handle();

private:
    handle _read_fd;
    handle _write_fd;
};

}
