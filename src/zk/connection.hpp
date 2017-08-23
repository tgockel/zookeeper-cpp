#pragma once

#include <zk/config.hpp>

#include <memory>
#include <mutex>
#include <vector>

#include "buffer.hpp"
#include "forwards.hpp"
#include "future.hpp"
#include "string_view.hpp"

namespace zk
{

class connection
{
public:
    static std::shared_ptr<connection> create(string_view conn_string);

    virtual ~connection() noexcept;

    virtual void close() = 0;

    virtual future<get_result> get(string_view path) = 0;

    virtual future<get_children_result> get_children(string_view path) = 0;

    virtual future<exists_result> exists(string_view path) = 0;

    virtual future<create_result> create(string_view     path,
                                         const buffer&   data,
                                         const acl_list& acls,
                                         create_mode     mode
                                        ) = 0;

    virtual future<set_result> set(string_view path, const buffer& data, version check) = 0;

    virtual future<void> erase(string_view path, version check) = 0;

    virtual future<multi_result> commit(multi_op&& txn) = 0;

    virtual future<void> load_fence() = 0;

    virtual zk::state state() const = 0;

    /** Watch for a state change. **/
    virtual future<zk::state> watch_state();

protected:
    /** Call this from derived classes when a session event happens. This triggers the delivery of all promises of state
     *  changes (issued through \c watch_state).
    **/
    virtual void on_session_event(zk::state new_state);

private:
    mutable std::mutex              _state_change_promises_protect;
    std::vector<promise<zk::state>> _state_change_promises;
};

}
