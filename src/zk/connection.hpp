#pragma once

#include <zk/config.hpp>

#include <memory>
#include <mutex>
#include <vector>

#include "client.hpp"
#include "future.hpp"
#include "string_view.hpp"
#include "watch.hpp"

namespace zk
{

class connection
{
public:
    static std::shared_ptr<connection> create(string_view conn_string);

    virtual ~connection() noexcept;

    virtual void close() = 0;

    virtual future<std::pair<buffer, stat>> get(string_view path) = 0;

    virtual future<std::pair<std::vector<std::string>, stat>> get_children(string_view path) = 0;

    virtual future<optional<stat>> exists(string_view path) = 0;

    virtual future<std::string> create(string_view     path,
                                       const buffer&   data,
                                       const acl_list& acls,
                                       create_mode     mode
                                      ) = 0;

    virtual future<stat> set(string_view path, const buffer& data, version check) = 0;

    virtual future<void> erase(string_view path, version check) = 0;

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
