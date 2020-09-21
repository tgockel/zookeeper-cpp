#include "connection_zk.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>

#include <zookeeper/zookeeper.h>

#include "acl.hpp"
#include "error.hpp"
#include "multi.hpp"
#include "results.hpp"
#include "types.hpp"

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility Functions                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename FAction>
auto with_str(string_view src, FAction&& action) noexcept(noexcept(std::forward<FAction>(action)(ptr<const char>())))
        -> decltype(std::forward<FAction>(action)(ptr<const char>()))
{
    char buffer[src.size() + 1];
    buffer[src.size()] = '\0';
    std::memcpy(buffer, src.data(), src.size());
    return std::forward<FAction>(action)(buffer);
}

static ACL encode_acl_part(const acl_rule& src)
{
    ACL out;
    out.perms     = static_cast<int>(src.permissions());
    out.id.scheme = const_cast<ptr<char>>(src.scheme().c_str());
    out.id.id     = const_cast<ptr<char>>(src.id().c_str());
    return out;
}

template <typename FAction>
auto with_acl(const acl& rules, FAction&& action) noexcept(noexcept(std::forward<FAction>(action)(ptr<ACL_vector>())))
        -> decltype(std::forward<FAction>(action)(ptr<ACL_vector>()))
{
    ACL parts[rules.size()];
    for (std::size_t idx = 0; idx < rules.size(); ++idx)
        parts[idx] = encode_acl_part(rules[idx]);

    ACL_vector vec;
    vec.count = int(rules.size());
    vec.data  = parts;
    return std::forward<FAction>(action)(&vec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Native Adaptors                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static error_code error_code_from_raw(int raw)
{
    switch (raw)
    {
    case ZOPERATIONTIMEOUT:
        raw = ZCONNECTIONLOSS;
        break;
    case ZINVALIDCALLBACK:
    case ZINVALIDACL:
        raw = ZBADARGUMENTS;
        break;
    case ZSESSIONMOVED:
        raw = ZCONNECTIONLOSS;
        break;
    default:
        break;
    }
    return static_cast<error_code>(raw);
}

static event_type event_from_raw(int raw)
{
    return static_cast<event_type>(raw);
}

// ZooKeeper does not have this concept pre-3.5
#if ZOO_MAJOR_VERSION <= 3 && ZOO_MINOR_VERSION <= 4
static const int ZOO_NOTCONNECTED_STATE = 999;
#endif

static state state_from_raw(int raw)
{
    // The C client will put us into `ZOO_NOTCONNECTED_STATE` for two reasons:
    //
    // 1. This is the state of the initial connection (zookeeper_init_internal), which is then replaced when the adaptor
    //    threads first call the interest function.
    // 2. During a reconfiguration, the client disconnects and transitions to this state (update_addrs), which is then
    //    updated the next time the I/O thread touches interest.
    //
    // In both cases, the state is still "connecting" from the point of view of a client.
    if (raw == ZOO_NOTCONNECTED_STATE)
    {
        raw = ZOO_CONNECTING_STATE;
    }
    // `ZOO_ASSOCIATING_STATE` means we have connected to a server, but have not yet authenticated and created the
    // session. We still can't perform any operations, so treat it as connecting -- the client does not care about the
    // difference between establishing a TCP connection and negotiating credentials.
    else if (raw == ZOO_ASSOCIATING_STATE)
    {
        raw = ZOO_CONNECTING_STATE;
    }

    return static_cast<state>(raw);
}

static stat stat_from_raw(const struct Stat& raw)
{
    stat out;
    out.acl_version = acl_version(raw.aversion);
    out.child_modified_transaction = transaction_id(raw.pzxid);
    out.child_version = child_version(raw.cversion);
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

static std::vector<std::string> string_vector_from_raw(const struct String_vector& raw)
{
    std::vector<std::string> out;
    out.reserve(raw.count);
    for (std::int32_t idx = 0; idx < raw.count; ++idx)
        out.emplace_back(raw.data[idx]);
    return out;
}

static acl acl_from_raw(const struct ACL_vector& raw)
{
    auto sz = std::size_t(raw.count);

    acl out;
    out.reserve(sz);
    for (std::size_t idx = 0; idx < sz; ++idx)
    {
        const auto& item = raw.data[idx];
        out.emplace_back(item.id.scheme, item.id.id, static_cast<permission>(item.perms));
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// connection_zk                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

connection_zk::connection_zk(const connection_params& params) :
        _handle(nullptr)
{
    if (params.connection_schema() != "zk")
        zk::throw_exception(std::invalid_argument(std::string("Invalid connection string \"") + to_string(params) + "\""));

    auto conn_string = [&] ()
                       {
                           std::ostringstream os;
                           bool first = true;
                           for (const auto& host : params.hosts())
                           {
                               if (first)
                                   first = false;
                               else
                                   os << ',';

                               os << host;
                           }
                           return os.str();
                       }();

    _handle = ::zookeeper_init(conn_string.c_str(),
                               on_session_event_raw,
                               static_cast<int>(params.timeout().count()),
                               nullptr,
                               this,
                               0
                              );

    if (!_handle)
        std::system_error(errno, std::system_category(), "Failed to create ZooKeeper client");
}

connection_zk::~connection_zk() noexcept
{
    close();
}

class connection_zk::watcher
{
public:
    watcher() :
            _event_delivered(false)
    { }

    virtual ~watcher() noexcept {}

    virtual void deliver_event(event ev)
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

protected:
    std::atomic<bool> _event_delivered;
    promise<event>    _event_promise;
};

template <typename TResult>
class connection_zk::basic_watcher :
        public connection_zk::watcher
{
public:
    basic_watcher() :
            _data_delivered(false)
    { }

    future<TResult> get_data_future()
    {
        return _data_promise.get_future();
    }

    virtual void deliver_event(event ev) override
    {
        if (!_data_delivered.load(std::memory_order_relaxed))
        {
            deliver_data(nullopt, get_exception_ptr_of(error_code::closed));
        }

        watcher::deliver_event(std::move(ev));
    }

    void deliver_data(optional<TResult> data, zk::exception_ptr ex_ptr)
    {
        if (!_data_delivered.exchange(true, std::memory_order_relaxed))
        {
            if (ex_ptr)
            {
                _data_promise.set_exception(std::move(ex_ptr));
            }
            else
            {
                _data_promise.set_value(std::move(*data));
            }
        }
    }

private:
    std::atomic<bool> _data_delivered;
    promise<TResult>  _data_promise;
};

std::shared_ptr<connection_zk::watcher> connection_zk::try_extract_watch(ptr<const void> addr)
{
    std::unique_lock<std::mutex> ax(_watches_protect);
    auto iter = _watches.find(addr);
    if (iter != _watches.end())
        return _watches.extract(iter).mapped();
    else
        return nullptr;
}

static ptr<connection_zk> connection_from_context(ptr<zhandle_t> zh)
{
    return (ptr<connection_zk>) zoo_get_context(zh);
}

void connection_zk::deliver_watch(ptr<zhandle_t>  zh,
                                  int             type_in,
                                  int             state_in,
                                  ptr<const char> path     [[gnu::unused]],
                                  ptr<void>       proms_in
                                 )
{
    auto& self = *connection_from_context(zh);
    if (auto watcher = self.try_extract_watch(proms_in))
        watcher->deliver_event(event(event_from_raw(type_in), state_from_raw(state_in)));
}

void connection_zk::close()
{
    if (_handle)
    {
        auto err = error_code_from_raw(::zookeeper_close(_handle));
        if (err != error_code::ok)
            throw_error(err);

        _handle = nullptr;

        // Deliver a session event as if there was a close.
        std::unique_lock<std::mutex> ax(_watches_protect);
        auto l_watches = std::move(_watches);
        ax.unlock();
        for (const auto& pair : l_watches)
            pair.second->deliver_event(event(event_type::session, zk::state::closed));
    }
}

zk::state connection_zk::state() const
{
    if (_handle)
        return state_from_raw(::zoo_state(_handle));
    else
        return zk::state::closed;
}

future<get_result> connection_zk::get(string_view path)
{
    ::data_completion_t callback =
        [] (int rc_in, ptr<const char> data, int data_sz, ptr<const struct Stat> pstat, ptr<const void> prom_in) noexcept
        {
            std::unique_ptr<promise<get_result>> prom((ptr<promise<get_result>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(get_result(buffer(data, data + data_sz), stat_from_raw(*pstat)));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        auto ppromise = std::make_unique<promise<get_result>>();
        auto fut      = ppromise->get_future();
        auto rc = error_code_from_raw(::zoo_aget(_handle, path, 0, callback, ppromise.get()));
        if (rc == error_code::ok)
        {
            ppromise.release();
            return fut;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return fut;
        }
    });
}

class connection_zk::data_watcher :
        public connection_zk::basic_watcher<watch_result>
{
public:
    static void deliver_raw(int                    rc_in,
                            ptr<const char>        data,
                            int                    data_sz,
                            ptr<const struct Stat> pstat,
                            ptr<const void>        self_in
                           ) noexcept
    {
        auto& self = *static_cast<ptr<data_watcher>>(const_cast<ptr<void>>(self_in));
        auto  rc   = error_code_from_raw(rc_in);

        if (rc == error_code::ok)
        {
            self.deliver_data(watch_result(get_result(buffer(data, data + data_sz), stat_from_raw(*pstat)),
                                           self.get_event_future()
                                          ),
                              zk::exception_ptr()
                             );
        }
        else
        {
            self.deliver_data(nullopt, get_exception_ptr_of(rc));
        }
    }
};

future<watch_result> connection_zk::watch(string_view path)
{
    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        std::unique_lock<std::mutex> ax(_watches_protect);
        auto watcher = std::make_shared<data_watcher>();
        auto rc      = error_code_from_raw(::zoo_awget(_handle,
                                                       path,
                                                       deliver_watch,
                                                       watcher.get(),
                                                       data_watcher::deliver_raw,
                                                       watcher.get()
                                                      )
                                          );
        if (rc == error_code::ok)
            _watches.emplace(watcher.get(), watcher);
        else
            watcher->deliver_data(nullopt, get_exception_ptr_of(rc));

        return watcher->get_data_future();
    });
}

future<get_children_result> connection_zk::get_children(string_view path)
{
    ::strings_stat_completion_t callback =
        [] (int                             rc_in,
            ptr<const struct String_vector> strings_in,
            ptr<const struct Stat>          stat_in,
            ptr<const void>                 prom_in
           )
        {
            std::unique_ptr<promise<get_children_result>> prom((ptr<promise<get_children_result>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            try
            {
                if (rc != error_code::ok)
                    throw_error(rc);

                prom->set_value(get_children_result(string_vector_from_raw(*strings_in), stat_from_raw(*stat_in)));
            }
            catch (...)
            {
                prom->set_exception(zk::current_exception());
            }
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        auto ppromise = std::make_unique<promise<get_children_result>>();
        auto fut      = ppromise->get_future();
        auto rc       = error_code_from_raw(::zoo_aget_children2(_handle,
                                                                 path,
                                                                 0,
                                                                 callback,
                                                                 ppromise.get()
                                                                )
                                           );
        if (rc == error_code::ok)
        {
            ppromise.release();
            return fut;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return fut;
        }
    });
}

class connection_zk::child_watcher :
        public connection_zk::basic_watcher<watch_children_result>
{
public:
    static void deliver_raw(int                             rc_in,
                            ptr<const struct String_vector> strings_in,
                            ptr<const struct Stat>          stat_in,
                            ptr<const void>                 prom_in
                           ) noexcept
    {
        auto& self = *static_cast<ptr<child_watcher>>(const_cast<ptr<void>>(prom_in));
        auto  rc   = error_code_from_raw(rc_in);

        try
        {
            if (rc != error_code::ok)
                throw_error(rc);

            self.deliver_data(watch_children_result(get_children_result(string_vector_from_raw(*strings_in),
                                                                        stat_from_raw(*stat_in)
                                                                       ),
                                                    self.get_event_future()
                                                   ),
                              zk::exception_ptr()
                             );
        }
        catch (...)
        {
            self.deliver_data(nullopt, zk::current_exception());
        }

    }
};

future<watch_children_result> connection_zk::watch_children(string_view path)
{
    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        std::unique_lock<std::mutex> ax(_watches_protect);
        auto watcher = std::make_shared<child_watcher>();
        auto rc      = error_code_from_raw(::zoo_awget_children2(_handle,
                                                                 path,
                                                                 deliver_watch,
                                                                 watcher.get(),
                                                                 child_watcher::deliver_raw,
                                                                 watcher.get()
                                                                )
                                          );
        if (rc == error_code::ok)
            _watches.emplace(watcher.get(), watcher);
        else
            watcher->deliver_data(nullopt, get_exception_ptr_of(rc));

        return watcher->get_data_future();
    });
}

future<exists_result> connection_zk::exists(string_view path)
{
    ::stat_completion_t callback =
        [] (int rc_in, ptr<const struct Stat> stat_in, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<exists_result>> prom((ptr<promise<exists_result>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(exists_result(stat_from_raw(*stat_in)));
            else if (rc == error_code::no_entry)
                prom->set_value(exists_result(nullopt));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        auto ppromise = std::make_unique<promise<exists_result>>();
        auto fut      = ppromise->get_future();
        auto rc       = error_code_from_raw(::zoo_aexists(_handle, path, 0, callback, ppromise.get()));
        if (rc == error_code::ok)
        {
            ppromise.release();
            return fut;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return fut;
        }
    });
}

class connection_zk::exists_watcher :
        public connection_zk::basic_watcher<watch_exists_result>
{
public:
    static void deliver_raw(int rc_in, ptr<const struct Stat> stat_in, ptr<const void> self_in)
    {
        auto& self = *static_cast<ptr<exists_watcher>>(const_cast<ptr<void>>(self_in));
        auto  rc   = error_code_from_raw(rc_in);

        if (rc == error_code::ok)
        {
            self.deliver_data(watch_exists_result(exists_result(stat_from_raw(*stat_in)), self.get_event_future()),
                              zk::exception_ptr()
                             );
        }
        else if (rc == error_code::no_entry)
        {
            self.deliver_data(watch_exists_result(exists_result(nullopt), self.get_event_future()),
                              zk::exception_ptr()
                             );
        }
        else
        {
            self.deliver_data(nullopt, get_exception_ptr_of(rc));
        }
    }
};

future<watch_exists_result> connection_zk::watch_exists(string_view path)
{
    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        std::unique_lock<std::mutex> ax(_watches_protect);
        auto watcher = std::make_shared<exists_watcher>();
        auto rc      = error_code_from_raw(::zoo_awexists(_handle,
                                                          path,
                                                          deliver_watch,
                                                          watcher.get(),
                                                          exists_watcher::deliver_raw,
                                                          watcher.get()
                                                         )
                                          );
        if (rc == error_code::ok)
            _watches.emplace(watcher.get(), watcher);
        else
            watcher->deliver_data(nullopt, get_exception_ptr_of(rc));

        return watcher->get_data_future();
    });
}

future<create_result> connection_zk::create(string_view   path,
                                            const buffer& data,
                                            const acl&    rules,
                                            create_mode   mode
                                           )
{
    ::string_completion_t callback =
        [] (int rc_in, ptr<const char> name_in, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<create_result>> prom((ptr<promise<create_result>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(create_result(std::string(name_in)));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

        return with_str(path, [&] (ptr<const char> path) noexcept
        {
            auto ppromise = std::make_unique<promise<create_result>>();
            auto fut      = ppromise->get_future();
            auto rc       = with_acl(rules, [&] (ptr<const ACL_vector> rules) noexcept
            {
                return error_code_from_raw(::zoo_acreate(_handle,
                                                         path,
                                                         data.data(),
                                                         int(data.size()),
                                                         rules,
                                                         static_cast<int>(mode),
                                                         callback,
                                                         ppromise.get()
                                                        )
                                          );
            });

            if (rc == error_code::ok)
            {
                ppromise.release();
                return fut;
            }
            else
            {
                ppromise->set_exception(get_exception_ptr_of(rc));
                return fut;
            }
        });
}

future<set_result> connection_zk::set(string_view path, const buffer& data, version check)
{
    ::stat_completion_t callback =
        [] (int rc_in, ptr<const struct Stat> stat_raw, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<set_result>> prom((ptr<promise<set_result>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(set_result(stat_from_raw(*stat_raw)));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        auto ppromise = std::make_unique<promise<set_result>>();
        auto fut      = ppromise->get_future();
        auto rc       = error_code_from_raw(::zoo_aset(_handle,
                                                       path,
                                                       data.data(),
                                                       int(data.size()),
                                                       check.value,
                                                       callback,
                                                       ppromise.get()
                                                      ));

        if (rc == error_code::ok)
        {
            ppromise.release();
            return fut;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return fut;
        }
    });
}

future<void> connection_zk::erase(string_view path, version check)
{
    ::void_completion_t callback =
        [] (int rc_in, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<void>> prom((ptr<promise<void>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value();
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        auto ppromise = std::make_unique<promise<void>>();
        auto fut      = ppromise->get_future();
        auto rc       = error_code_from_raw(::zoo_adelete(_handle, path, check.value, callback, ppromise.get()));
        if (rc == error_code::ok)
        {
            ppromise.release();
            return fut;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return fut;
        }
    });
}

future<get_acl_result> connection_zk::get_acl(string_view path) const
{
    ::acl_completion_t callback =
        [] (int rc_in, ptr<struct ACL_vector> acl_raw, ptr<struct Stat> stat_raw, ptr<const void> prom_in) noexcept
        {
            std::unique_ptr<promise<get_acl_result>> prom((ptr<promise<get_acl_result>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(get_acl_result(acl_from_raw(*acl_raw), stat_from_raw(*stat_raw)));
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        auto ppromise = std::make_unique<promise<get_acl_result>>();
        auto fut      = ppromise->get_future();
        auto rc       = error_code_from_raw(::zoo_aget_acl(_handle, path, callback, ppromise.get()));
        if (rc == error_code::ok)
        {
            ppromise.release();
            return fut;
        }
        else
        {
            ppromise->set_exception(get_exception_ptr_of(rc));
            return fut;
        }
    });
}

future<void> connection_zk::set_acl(string_view path, const acl& rules, acl_version check)
{
    ::void_completion_t callback =
        [] (int rc_in, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<void>> prom((ptr<promise<void>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value();
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path) noexcept
    {
        return with_acl(rules, [&] (ptr<struct ACL_vector> rules) noexcept
        {
            auto ppromise = std::make_unique<promise<void>>();
            auto fut      = ppromise->get_future();
            auto rc       = error_code_from_raw(::zoo_aset_acl(_handle,
                                                               path,
                                                               check.value,
                                                               rules,
                                                               callback,
                                                               ppromise.get()
                                                              )
                                               );
            if (rc == error_code::ok)
            {
                ppromise.release();
                return fut;
            }
            else
            {
                ppromise->set_exception(get_exception_ptr_of(rc));
                return fut;
            }
        });
    });
}

struct connection_zk_commit_completer
{
    multi_op                                 source_txn;
    promise<multi_result>                    prom;
    std::vector<zoo_op_result_t>             raw_results;
    std::map<std::size_t, Stat>              raw_stats;
    std::map<std::size_t, std::vector<char>> path_buffers;

    explicit connection_zk_commit_completer(multi_op&& src) :
            source_txn(std::move(src)),
            raw_results(source_txn.size())
    {
        for (zoo_op_result_t& x : raw_results)
            x.err = -42;
    }

    ptr<Stat> raw_stat_at(std::size_t idx)
    {
        return &raw_stats[idx];
    }

    ptr<std::vector<char>> path_buffer_for(std::size_t idx, const std::string& path, create_mode mode)
    {
        // If the creation is sequential, append 12 extra characters to store the digits
        auto sz = path.size() + (is_set(mode, create_mode::sequential) ? 12 : 1);
        path_buffers[idx] = std::vector<char>(sz);
        return &path_buffers[idx];
    }

    void deliver(error_code rc)
    {
        try
        {
            if (rc == error_code::ok)
            {
                multi_result out;
                out.reserve(raw_results.size());
                for (std::size_t idx = 0; idx < source_txn.size(); ++idx)
                {
                    const auto& raw_res = raw_results[idx];

                    switch (source_txn[idx].type())
                    {
                    case op_type::create:
                        out.emplace_back(create_result(std::string(raw_res.value)));
                        break;
                    case op_type::set:
                        out.emplace_back(set_result(stat_from_raw(*raw_res.stat)));
                        break;
                    default:
                        out.emplace_back(source_txn[idx].type(), nullptr);
                        break;
                    }
                }

                prom.set_value(std::move(out));
            }
            else
            {
                // All results until the failure are 0, equal to rc where we care, and runtime_inconsistency after that.
                auto iter = std::partition_point(raw_results.begin(), raw_results.end(),
                                                 [] (auto res) { return res.err == 0; }
                                                );
                zk::throw_exception(transaction_failed(rc, std::size_t(std::distance(raw_results.begin(), iter))));
            }
        }
        catch (...)
        {
            prom.set_exception(zk::current_exception());
        }
    }
};

future<multi_result> connection_zk::commit(multi_op&& txn_in)
{
    ::void_completion_t callback =
        [] (int rc_in, ptr<const void> completer_in)
        {
            std::unique_ptr<connection_zk_commit_completer>
                completer((ptr<connection_zk_commit_completer>) completer_in);
            completer->deliver(error_code_from_raw(rc_in));
        };

    auto pcompleter = std::make_unique<connection_zk_commit_completer>(std::move(txn_in));
    auto& txn = pcompleter->source_txn;
    try
    {
        ::zoo_op raw_ops[txn.size()];
        std::size_t create_op_count = 0;
        std::size_t acl_piece_count = 0;
        for (const auto& tx : txn)
        {
            if (tx.type() == op_type::create)
            {
                ++create_op_count;
                acl_piece_count += tx.as_create().rules.size();
            }
        }
        ACL_vector      encoded_acls[create_op_count];
        ACL             acl_pieces[acl_piece_count];
        ptr<ACL_vector> encoded_acl_iter = encoded_acls;
        ptr<ACL>        acl_piece_iter   = acl_pieces;

        for (std::size_t idx = 0; idx < txn.size(); ++idx)
        {
            auto& raw_op = raw_ops[idx];
            auto& src_op = txn[idx];
            switch (src_op.type())
            {
                case op_type::check:
                    zoo_check_op_init(&raw_op, src_op.as_check().path.c_str(), src_op.as_check().check.value);
                    break;
                case op_type::create:
                {
                    const auto& cdata = src_op.as_create();
                    encoded_acl_iter->count = int(cdata.rules.size());
                    encoded_acl_iter->data  = acl_piece_iter;
                    for (const auto& acl : cdata.rules)
                    {
                        *acl_piece_iter = encode_acl_part(acl);
                        ++acl_piece_iter;
                    }

                    auto path_buf_ref = pcompleter->path_buffer_for(idx, cdata.path, cdata.mode);
                    zoo_create_op_init(&raw_op,
                                       cdata.path.c_str(),
                                       cdata.data.data(),
                                       int(cdata.data.size()),
                                       encoded_acl_iter,
                                       static_cast<int>(cdata.mode),
                                       path_buf_ref->data(),
                                       int(path_buf_ref->size())
                                      );
                    ++encoded_acl_iter;
                    break;
                }
                case op_type::erase:
                    zoo_delete_op_init(&raw_op, src_op.as_erase().path.c_str(), src_op.as_erase().check.value);
                    break;
                case op_type::set:
                {
                    const auto& setting = src_op.as_set();
                    zoo_set_op_init(&raw_op,
                                    setting.path.c_str(),
                                    setting.data.data(),
                                    int(setting.data.size()),
                                    setting.check.value,
                                    pcompleter->raw_stat_at(idx)
                                   );
                    break;
                }
                default:
                {
                    using std::to_string;
                    zk::throw_exception(std::invalid_argument("Invalid op_type at index=" + to_string(idx) + ": "
                                                + to_string(src_op.type())
                                               ));
                }
            }
        }
        auto fut = pcompleter->prom.get_future();
        auto rc = error_code_from_raw(::zoo_amulti(_handle,
                                                   int(txn.size()),
                                                   raw_ops,
                                                   pcompleter->raw_results.data(),
                                                   callback,
                                                   pcompleter.get()
                                                  )
                                     );
        if (rc == error_code::ok)
        {
            pcompleter.release();
            return fut;
        }
        else
        {
            pcompleter->prom.set_exception(get_exception_ptr_of(rc));
            return pcompleter->prom.get_future();
        }
    }
    catch (...)
    {
        pcompleter->prom.set_exception(zk::current_exception());
        return pcompleter->prom.get_future();
    }
}

future<void> connection_zk::load_fence()
{
    ::string_completion_t callback =
        [] (int rc_in, ptr<const char>, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<void>> prom((ptr<promise<void>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value();
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    auto ppromise = std::make_unique<zk::promise<void>>();
    auto rc = error_code_from_raw(::zoo_async(_handle, "/", callback, ppromise.get()));
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
}

void connection_zk::on_session_event_raw(ptr<zhandle_t>  handle      [[gnu::unused]],
                                         int             ev_type,
                                         int             state,
                                         ptr<const char> path_ptr,
                                         ptr<void>       watcher_ctx
                                        ) noexcept
{
    auto self = static_cast<ptr<connection_zk>>(watcher_ctx);
    // Most of the time, self's _handle will be the same thing that ZK provides to us. However, if we connect very
    // quickly, a session event will happen trigger *before* we set the _handle. This isn't a problem, just something to
    // be aware of.
    assert(self->_handle == nullptr || self->_handle == handle);
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
