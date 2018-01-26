#pragma once

#include <zk/config.hpp>

namespace zk::detail
{

class event_handle final
{
public:
    using native_handle_type = int;

public:
    explicit event_handle();

    event_handle(const event_handle&) = delete;

    event_handle& operator=(const event_handle&) = delete;

    ~event_handle() noexcept;

    /** Close this event for future signalling. This is automatically called from the destructor. **/
    void close() noexcept;

    /** Signal this handle that something has happened. **/
    void notify_one();

    /** Attempt to wait for this handle to be signalled, but do not block.
     *
     *  \returns \c true if we successfully waited for a signal (and consumed it); \c false if this handle was not
     *   signalled.
    **/
    bool try_wait();

    /** Get the file descriptor backing this handle. This is generally only used when interacting with the kernel and
     *  should be avoided in regular use.
    **/
    native_handle_type native_handle();

private:
    native_handle_type _fd;
};

}
