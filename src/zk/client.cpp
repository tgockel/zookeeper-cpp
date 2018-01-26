#include "client.hpp"
#include "acl.hpp"
#include "connection.hpp"
#include "detail/event_handle.hpp"
#include "multi.hpp"
#include "notification.hpp"

#include <array>
#include <iostream>
#include <sstream>
#include <ostream>
#include <unordered_set>

#include <sys/select.h>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// trackers                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{

class tracker
{
public:
    virtual ~tracker() noexcept
    { }

    virtual void deliver(notification item) = 0;
};

template <typename TResult>
class basic_tracker :
        public tracker
{
public:
    basic_tracker() :
            _data_delivered(false)
    { }

    future<TResult> get_data_future()
    {
        return _data_promise.get_future();
    }

    virtual void deliver(notification item) override
    {
        if (item.is<std::exception_ptr>())
        {
            _data_promise.set_exception(std::move(item.as<std::exception_ptr>()));
            return;
        }
        else if (item.is<event>())
        {
            auto& ev = item.as<event>();
            if (ev.type() == event_type::session)
            {
                _data_promise.set_exception(get_exception_ptr_of(error_code::closed));
                return;
            }
        }
        else if (item.is<TResult>())
        {
            _data_promise.set_value(std::move(item.as<TResult>()));
            return;
        }

        std::cerr << "ZK CRITICAL: Unhandled notification " << item << std::endl;
    }

private:
    promise<TResult> _data_promise;
};

template <typename TResult>
class basic_watcher :
        public basic_tracker<TResult>
{
    using base_type = basic_tracker<TResult>;

public:
    basic_watcher() :
            _event_delivered(false)
    { }

    virtual void deliver(notification item) override
    {
        if (!_event_delivered.exchange(true, std::memory_order_relaxed))
        {
            _event_promise.set_value(std::move(ev));
        }
    }

    future<event> get_event_future()
    {
        return _event_promise.get_future();
    }

private:
    std::atomic<bool> _event_delivered;
    promise<event>    _event_promise;
};

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client::impl                                                                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct client::impl
{
    std::shared_ptr<connection>   conn;
    std::atomic<bool>             is_running;
    detail::event_handle          is_running_alert;
    std::unordered_set<ptr<void>> trackers;
    mutable std::mutex            trackers_protect;
    std::thread                   worker;

    explicit impl(std::shared_ptr<connection> conn);

    ~impl() noexcept;

    void close(bool wait_for_stop);

    void run();

    template <typename TResult, typename TOp>
    future<TResult> submit(TOp&& operation);

private:
    void deliver(notification item);
};

client::impl::impl(std::shared_ptr<connection> conn_) :
        conn(std::move(conn_)),
        is_running(true),

        worker([this] { this->run(); })
{ }

client::impl::~impl() noexcept
{
    close(true);
}

void client::impl::close(bool wait_for_stop)
{
    is_running.store(false, std::memory_order_release);
    is_running_alert.notify_one();

    if (wait_for_stop && worker.joinable())
        worker.join();
}

static void wait_for_event(int fd1, int fd2)
{
    // This could be implemented with epoll instead of select, but since N=2, it doesn't really matter
    struct fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd1, &read_fds);
    FD_SET(fd2, &read_fds);

    if (::select(2, &read_fds, nullptr, nullptr, nullptr) < 0)
    {
        if (errno == EINTR)
            return;
        else
            throw std::system_error(errno, std::system_category(), "select");
    }
}

void client::impl::run()
{
    std::array<notification, 64U> notifications;

    while (is_running.load(std::memory_order_acquire))
    {
        wait_for_event(conn->native_handle(), is_running_alert.native_handle());

        auto read_count = conn.receive(notifications.data(), notifications.size());
        if (read_count == 0U)
            continue;

        for (auto idx = 0UL; idx < read_count; ++idx)
        {
            deliver(std::move(notifications[idx]));
        }
    }
}

// template <typename TResult, typename TOp>
// future<TResult> client::impl::submit(TOp&&)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client                                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

client::client(const connection_params& params) :
        client(connection::connect(params))
{ }

client::client(string_view conn_string) :
        client(connection::connect(conn_string))
{ }

client::client(std::shared_ptr<connection> conn) noexcept :
        _impl(std::make_shared<impl>(std::move(conn)))
{ }

future<client> client::connect(string_view conn_string)
{
    return connect(connection_params::parse(conn_string));
}

future<client> client::connect(connection_params conn_params)
{
    try
    {
        auto conn = connection::connect(conn_params);
        auto state_change_fut = conn->watch_state();
        if (conn->state() == state::connected)
        {
            promise<client> p;
            p.set_value(client(std::move(conn)));
            return p.get_future();
        }
        else
        {
            // TODO: Test if future::then can be relied on and use that instead of std::async
            return std::async
                   (
                       std::launch::async,
                       [state_change_fut = std::move(state_change_fut), conn = std::move(conn)] () mutable -> client
                       {
                         state s(state_change_fut.get());
                         if (s == state::connected)
                            return client(conn);
                         else
                             throw std::runtime_error(std::string("Unexpected state: ") + to_string(s));
                       }
                   );
        }
    }
    catch (...)
    {
        promise<client> p;
        p.set_exception(std::current_exception());
        return p.get_future();
    }
}

client::~client() noexcept
{
    close(true);
}

void client::close(bool wait_for_stop)
{
    if (auto conn = std::exchange(_conn, nullptr))
        conn->close(wait_for_stop);
}

future<get_result> client::get(string_view path) const
{
    return _impl->submit<get_result>(op::get(std::string(path)));
}

future<watch_result> client::watch(string_view path) const
{
    return _conn->watch(path);
}

future<get_children_result> client::get_children(string_view path) const
{
    return _impl->submit<get_children_result>(op::get_children(std::string(path)));
}

future<watch_children_result> client::watch_children(string_view path) const
{
    return _conn->watch_children(path);
}

future<exists_result> client::exists(string_view path) const
{
    return _impl->submit<exists_result>(op::exists(std::string(path)));
}

future<watch_exists_result> client::watch_exists(string_view path) const
{
    return _conn->watch_exists(path);
}

future<create_result> client::create(string_view   path,
                                     const buffer& data,
                                     const acl&    rules,
                                     create_mode   mode
                                    )
{
    return _impl->submit<create_result>(op::create(std::string(path), data, rules, mode));
}

future<create_result> client::create(string_view   path,
                                     const buffer& data,
                                     create_mode   mode
                                    )
{
    return create(path, data, acls::open_unsafe(), mode);
}

future<set_result> client::set(string_view path, const buffer& data, version check)
{
    return _impl->submit<set_result>(std::string(path), data, check);
}

future<get_acl_result> client::get_acl(string_view path) const
{
    return _impl->submit<get_acl_result>(std::string(path));
}

future<void> client::set_acl(string_view path, const acl& rules, acl_version check)
{
    return _impl->submit<void>(op::set_acl(std::string(path), rules, check));
}

future<void> client::erase(string_view path, version check)
{
    return _conn->erase(path, check);
}

future<void> client::load_fence() const
{
    return _conn->load_fence();
}

future<multi_result> client::commit(multi_op txn)
{
    return _conn->commit(std::move(txn));
}

}
