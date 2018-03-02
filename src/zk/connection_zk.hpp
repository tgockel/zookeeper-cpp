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

    virtual future<get_result> get(string_view path) override;

    virtual future<watch_result> watch(string_view path) override;

    virtual future<get_children_result> get_children(string_view path) override;

    virtual future<watch_children_result> watch_children(string_view path) override;

    virtual future<exists_result> exists(string_view path) override;

    virtual future<watch_exists_result> watch_exists(string_view path) override;

    virtual future<create_result> create(string_view   path,
                                         const buffer& data,
                                         const acl&    rules,
                                         create_mode   mode
                                        ) override;

    virtual future<set_result> set(string_view path, const buffer& data, version check) override;

    virtual future<void> erase(string_view path, version check) override;

    virtual future<get_acl_result> get_acl(string_view path) const override;

    virtual future<void> set_acl(string_view path, const acl& rules, acl_version check) override;

    virtual future<multi_result> commit(multi_op&& txn) override;

    virtual future<void> load_fence() override;

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
    std::unordered_map<ptr<const void>, std::shared_ptr<watcher>> _watches;
    mutable std::mutex                                            _watches_protect;
};

/// \}

}
