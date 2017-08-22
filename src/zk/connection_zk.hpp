#pragma once

#include <zk/config.hpp>

#include <chrono>

#include "connection.hpp"
#include "string_view.hpp"

typedef struct _zhandle zhandle_t;

namespace zk
{

class connection_zk final :
        public connection
{
public:
    explicit connection_zk(string_view               conn_string,
                           std::chrono::milliseconds recv_timeout = std::chrono::milliseconds(10000)
                          );

    virtual ~connection_zk() noexcept;

    virtual void close() override;

    virtual zk::state state() const override;

    virtual future<std::pair<buffer, stat>> get(string_view path) override;

    virtual future<std::pair<std::vector<std::string>, stat>> get_children(string_view path) override;

    virtual future<std::string> create(string_view     path,
                                       const buffer&   data,
                                       const acl_list& acl,
                                       create_mode     mode
                                      ) override;

    virtual future<stat> set(string_view path, const buffer& data, version check) override;

    virtual future<void> erase(string_view path, version check) override;

private:
    static void on_session_event_raw(ptr<zhandle_t>  handle,
                                     int             ev_type,
                                     int             state,
                                     ptr<const char> path,
                                     ptr<void>       watcher_ctx
                                    ) noexcept;

private:
    ptr<zhandle_t> _handle;
};

}
