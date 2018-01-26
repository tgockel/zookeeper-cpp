#pragma once

#include <zk/config.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "connection.hpp"
#include "string_view.hpp"

typedef struct _zhandle zhandle_t;

namespace zk
{

/// \addtogroup Client
/// \{

class connection_zk final :
        public connection
{
public:
    explicit connection_zk(const connection_params& params);

    virtual ~connection_zk() noexcept;

    virtual void close() override;

    virtual zk::state state() const override;

    virtual void submit(op,       std::shared_ptr<void> track) override;
    virtual void submit(watch_op, std::shared_ptr<void> track) override;
    virtual void submit(multi_op, std::shared_ptr<void> track) override;

    virtual size_type receive(ptr<notification> out, size_type max) noexcept override;

    virtual native_handle_type native_handle() override;

private:
    static void on_session_event_raw(ptr<zhandle_t>  handle,
                                     int             ev_type,
                                     int             state,
                                     ptr<const char> path,
                                     ptr<void>       watcher_ctx
                                    ) noexcept;

    using watch_function = void (*)(ptr<zhandle_t>, int type_in, int state_in, ptr<const char>, ptr<void>);

    class watcher;

    template <typename TResult>
    class basic_watcher;

    class data_watcher;

    class child_watcher;

    class exists_watcher;

    /** Erase the watch tracker for the watch with the value \a p.
     *
     *  \returns \c true if it was deleted (the watch should be delivered); \c false if \a p was not in the list.
    **/
    std::shared_ptr<watcher> try_extract_watch(ptr<const void> p);

    static void deliver_watch(ptr<zhandle_t> zh, int type_in, int state_in, ptr<const char>, ptr<void> proms_in);

private:
    ptr<zhandle_t>                                                _handle;
    native_handle_type                                            _receive_handle;
    std::unordered_map<ptr<const void>, std::shared_ptr<watcher>> _watches;
    mutable std::mutex                                            _watches_protect;
};

/// \}

}
