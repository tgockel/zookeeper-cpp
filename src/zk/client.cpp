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

client::~client() noexcept = default;

void client::close()
{
    _conn->close();
}

}
