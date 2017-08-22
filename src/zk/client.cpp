#include "client.hpp"
#include "connection.hpp"
#include "acl.hpp"

#include <sstream>
#include <ostream>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create_mode                                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const create_mode& mode)
{
    if (mode == create_mode::normal)
        return os << "normal";

    bool first = true;
    auto tick = [&] { return std::exchange(first, false) ? "" : "|"; };
    if (is_set(mode, create_mode::ephemeral))  os << tick() << "ephemeral";
    if (is_set(mode, create_mode::sequential)) os << tick() << "sequential";
    if (is_set(mode, create_mode::container))  os << tick() << "container";

    return os;
}

std::string to_string(const create_mode& self)
{
    std::ostringstream os;
    os << self;
    return os.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client                                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

client::client(string_view conn_string) :
        client(connection::create(conn_string))
{ }

client::client(std::shared_ptr<connection> conn) noexcept :
        _conn(std::move(conn))
{ }

future<client> client::connect(string_view conn_string)
{
    try
    {
        auto conn = connection::create(conn_string);
        auto state_change_fut = conn->watch_state();
        if (conn->state() == state::connected)
        {
            promise<client> p;
            p.set_value(client(std::move(conn)));
            return p.get_future();
        }
        else
        {
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

client::~client() noexcept = default;

void client::close()
{
    _conn->close();
}

future<std::pair<buffer, stat>> client::get(string_view path) const
{
    return _conn->get(path);
}

future<std::string> client::create(string_view     path,
                                   const buffer&   data,
                                   const acl_list& acls,
                                   create_mode     mode
                                  )
{
    return _conn->create(path, data, acls, mode);
}

future<std::string> client::create(string_view     path,
                                   const buffer&   data,
                                   create_mode     mode
                                  )
{
    return create(path, data, acls::open_unsafe(), mode);
}

future<stat> client::set(string_view path, const buffer& data, version check)
{
    return _conn->set(path, data, check);
}

}
