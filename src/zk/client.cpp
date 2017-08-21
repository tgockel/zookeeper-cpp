#include "client.hpp"
#include "connection.hpp"

namespace zk
{

client::client(string_view conn_string) :
        client(connection::create(conn_string))
{ }

client::client(std::shared_ptr<connection> conn) noexcept :
        _conn(std::move(conn))
{ }

future<client> client::create(string_view conn_string)
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

}
