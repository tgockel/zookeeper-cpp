#include "connection_zk.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include <zookeeper/zookeeper.h>

#include "acl.hpp"
#include "error.hpp"
#include "stat.hpp"
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

template <typename FAction>
auto with_acl(const acl_list& acls, FAction&& action)
        -> decltype(std::forward<FAction>(action)(ptr<const ACL_vector>()))
{
    ACL parts[acls.size()];
    for (std::size_t idx = 0; idx < acls.size(); ++idx)
    {
        parts[idx].perms     = static_cast<int>(acls[idx].permissions());
        parts[idx].id.scheme = const_cast<ptr<char>>(acls[idx].scheme().c_str());
        parts[idx].id.id     = const_cast<ptr<char>>(acls[idx].id().c_str());
    }

    ACL_vector vec;
    vec.count = int(acls.size());
    vec.data  = parts;
    return std::forward<FAction>(action)(&vec);
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

static stat stat_from_raw(const struct Stat& raw)
{
    stat out;
    out.acl_version = version(raw.aversion);
    out.child_modified_transaction = transaction_id(raw.pzxid);
    out.child_version = version(raw.cversion);
    out.children_count = raw.numChildren;
    out.create_time = stat::time_point() + std::chrono::milliseconds(raw.ctime);
    out.create_transaction = transaction_id(raw.czxid);
    out.data_size = raw.dataLength;
    out.data_version = version(raw.version);
    out.ephemeral_owner = raw.ephemeralOwner;
    out.modified_time = stat::time_point() + std::chrono::milliseconds(raw.mtime);
    out.modified_transaction = transaction_id(raw.mzxid);
    return out;
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
                                              on_session_event_raw,
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

zk::state connection_zk::state() const
{
    if (_handle)
        return state_from_raw(::zoo_state(_handle));
    else
        return zk::state::closed;
}

future<std::pair<buffer, stat>> connection_zk::get(string_view path)
{
    ::data_completion_t callback =
        [] (int rc_in, ptr<const char> data, int data_sz, ptr<const struct Stat> pstat, ptr<const void> prom_in) noexcept
        {
            std::unique_ptr<promise<std::pair<buffer, stat>>> prom((ptr<promise<std::pair<buffer, stat>>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value({ buffer(data, data + data_sz), stat_from_raw(*pstat) });
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path)
    {
        auto ppromise = std::make_unique<promise<std::pair<buffer, stat>>>();
        auto rc = error_code_from_raw(::zoo_aget(_handle, path, 0, callback, ppromise.get()));
        if (rc == error_code::ok)
        {
            auto f = ppromise->get_future();
            ppromise.release();
            return f;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return ppromise->get_future();
        }
    });
}

future<std::string> connection_zk::create(string_view     path,
                                          const buffer&   data,
                                          const acl_list& acl,
                                          create_mode     mode
                                         )
{
    ::string_completion_t callback =
        [] (int rc_in, ptr<const char> name_in, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<std::string>> prom((ptr<promise<std::string>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(std::string(name_in));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

        return with_str(path, [&] (ptr<const char> path)
        {
            auto ppromise = std::make_unique<promise<std::string>>();
            auto rc = with_acl(acl, [&] (ptr<const ACL_vector> acl)
            {
                return error_code_from_raw(::zoo_acreate(_handle,
                                                         path,
                                                         data.data(),
                                                         int(data.size()),
                                                         acl,
                                                         static_cast<int>(mode),
                                                         callback,
                                                         ppromise.get()
                                                        ));
            });
            if (rc == error_code::ok)
            {
                auto f = ppromise->get_future();
                ppromise.release();
                return f;
            }
            else
            {
                ppromise->set_exception(get_exception_ptr_of(rc));
                return ppromise->get_future();
            }
        });
}

future<stat> connection_zk::set(string_view path, const buffer& data, version check)
{
    ::stat_completion_t callback =
        [] (int rc_in, ptr<const struct Stat> stat_raw, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<stat>> prom((ptr<promise<stat>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(stat_from_raw(*stat_raw));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path)
    {
        auto ppromise = std::make_unique<promise<stat>>();
        auto rc = error_code_from_raw(::zoo_aset(_handle,
                                                 path,
                                                 data.data(),
                                                 int(data.size()),
                                                 check.value,
                                                 callback,
                                                 ppromise.get()
                                                ));
        if (rc == error_code::ok)
        {
            auto f = ppromise->get_future();
            ppromise.release();
            return f;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return ppromise->get_future();
        }
    });
}

void connection_zk::on_session_event_raw(ptr<zhandle_t>  handle,
                                         int             ev_type,
                                         int             state,
                                         ptr<const char> path_ptr,
                                         ptr<void>       watcher_ctx
                                        ) noexcept
{
    auto self = static_cast<ptr<connection_zk>>(watcher_ctx);
    assert(self->_handle == handle);
    auto ev = event_from_raw(ev_type);
    auto st = state_from_raw(state);
    auto path = string_view(path_ptr);
    if (ev != event_type::session)
    {
        // TODO: Remove this usage of std::cerr
        std::cerr << "WARNING: Got unexpected event " << ev << " in state=" << st << " with path=" << path << std::endl;
        return;
    }
    self->on_session_event(st);
}

}
