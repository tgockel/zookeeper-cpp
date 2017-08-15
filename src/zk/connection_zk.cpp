#include "connection_zk.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <zookeeper/zookeeper.h>

#include "acl.hpp"
#include "error.hpp"
#include "watch.hpp"

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility Functions                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename FAction>
auto with_str(string_view src, FAction&& action)
        -> decltype(std::forward<FAction>(action)(ptr<const char>()))
{
    char buffer[src.size() + 1];
    buffer[src.size()] = '\0';
    std::memcpy(buffer, src.data(), src.size());
    return std::forward<FAction>(action)(buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Native Adaptors                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static error_code error_code_from_raw(int raw)
{
    return static_cast<error_code>(raw);
}

static event_type event_from_raw(int raw)
{
    return static_cast<event_type>(raw);
}

static state state_from_raw(int raw)
{
    return static_cast<state>(raw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// connection_zk                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

connection_zk::connection_zk(string_view conn_string, std::chrono::milliseconds recv_timeout) :
        _handle(nullptr)
{
    if (conn_string.find("zk://") != 0U)
        throw std::invalid_argument(std::string("Invalid connection string \"") + std::string(conn_string) + "\"");
    conn_string.remove_prefix(5);

    _handle = with_str
              (
                  conn_string,
                  [&] (ptr<const char> conn_c_string)
                  {
                      return ::zookeeper_init(conn_c_string,
                                              on_session_event,
                                              static_cast<int>(recv_timeout.count()),
                                              nullptr,
                                              this,
                                              0
                                             );
                  }
              );
    if (!_handle)
        std::system_error(errno, std::system_category(), "Failed to create ZooKeeper client");
}

connection_zk::~connection_zk() noexcept
{
    close();
}

void connection_zk::close()
{
    if (_handle)
    {
        auto err = error_code_from_raw(::zookeeper_close(_handle));
        if (err != error_code::ok)
            throw_error(err);

        _handle = nullptr;
    }
}

void connection_zk::on_session_event(ptr<zhandle_t>  handle,
                                     int             ev_type,
                                     int             state,
                                     ptr<const char> path_ptr,
                                     ptr<void>       watcher_ctx
                                    ) noexcept
{
    auto self = static_cast<ptr<connection_zk>>(watcher_ctx);
    assert(self->_handle == handle);
    event_from_raw(ev_type);
    state_from_raw(state);
    auto path = string_view(path_ptr);
    static_cast<void>(path);
}

}
