#include "connection_zk.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include <zookeeper/zookeeper.h>

#include "acl.hpp"
#include "error.hpp"
#include "multi.hpp"
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

static ACL encode_acl_part(const acl& src)
{
    ACL out;
    out.perms     = static_cast<int>(src.permissions());
    out.id.scheme = const_cast<ptr<char>>(src.scheme().c_str());
    out.id.id     = const_cast<ptr<char>>(src.id().c_str());
    return out;
}

template <typename FAction>
auto with_acl(const acl_list& acls, FAction&& action)
        -> decltype(std::forward<FAction>(action)(ptr<const ACL_vector>()))
{
    ACL parts[acls.size()];
    for (std::size_t idx = 0; idx < acls.size(); ++idx)
        parts[idx] = encode_acl_part(acls[idx]);

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

static std::vector<std::string> string_vector_from_raw(const struct String_vector& raw)
{
    std::vector<std::string> out;
    out.reserve(raw.count);
    for (std::int32_t idx = 0; idx < raw.count; ++idx)
        out.emplace_back(raw.data[idx]);
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

future<std::pair<std::vector<std::string>, stat>> connection_zk::get_children(string_view path)
{
    ::strings_stat_completion_t callback =
        [] (int                             rc_in,
            ptr<const struct String_vector> strings_in,
            ptr<const struct Stat>          stat_in,
            ptr<const void>                 prom_in
           )
        {
            std::unique_ptr<promise<std::pair<std::vector<std::string>, stat>>>
                prom((ptr<promise<std::pair<std::vector<std::string>, stat>>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
            {
                try
                {
                    prom->set_value({ string_vector_from_raw(*strings_in), stat_from_raw(*stat_in) });
                }
                catch (...)
                {
                    prom->set_exception(std::current_exception());
                }
            }
            else
            {
                prom->set_exception(get_exception_ptr_of(rc));
            }
        };

    return with_str(path, [&] (ptr<const char> path)
    {
        auto ppromise = std::make_unique<promise<std::pair<std::vector<std::string>, stat>>>();
        auto rc = error_code_from_raw(::zoo_aget_children2(_handle,
                                                           path,
                                                           0,
                                                           callback,
                                                           ppromise.get()
                                                          )
                                     );
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

future<optional<stat>> connection_zk::exists(string_view path)
{
    ::stat_completion_t callback =
        [] (int rc_in, ptr<const struct Stat> stat_in, ptr<const void> prom_in)
        {
            std::unique_ptr<promise<optional<stat>>> prom((ptr<promise<optional<stat>>>) prom_in);
            auto rc = error_code_from_raw(rc_in);
            if (rc == error_code::ok)
                prom->set_value(stat_from_raw(*stat_in));
            else if (rc == error_code::no_node)
                prom->set_value(nullopt);
            else
                prom->set_exception(get_exception_ptr_of(rc));
        };

    return with_str(path, [&] (ptr<const char> path)
    {
        auto ppromise = std::make_unique<promise<optional<stat>>>();
        auto rc = error_code_from_raw(::zoo_aexists(_handle, path, 0, callback, ppromise.get()));
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

    return with_str(path, [&] (ptr<const char> path)
    {
        auto ppromise = std::make_unique<promise<void>>();
        auto rc = error_code_from_raw(::zoo_adelete(_handle, path, check.value, callback, ppromise.get()));
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
    { }

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
                prom.set_value(multi_result());
            else
                throw_error(rc);
        }
        catch (...)
        {
            prom.set_exception(std::current_exception());
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
                acl_piece_count += tx.as_create().acl.size();
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
                    encoded_acl_iter->count = int(cdata.acl.size());
                    encoded_acl_iter->data  = acl_piece_iter;
                    for (const auto& acl : cdata.acl)
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
                    throw std::invalid_argument("Invalid op_type at index=" + to_string(idx) + ": "
                                                + to_string(src_op.type())
                                               );
                }
            }
        }
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
            auto f = pcompleter->prom.get_future();
            pcompleter.release();
            return f;
        }
        else
        {
            pcompleter->prom.set_exception(get_exception_ptr_of(rc));
            return pcompleter->prom.get_future();
        }
    }
    catch (...)
    {
        pcompleter->prom.set_exception(std::current_exception());
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

    auto ppromise = std::make_unique<std::promise<void>>();
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
