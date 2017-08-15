#include "connection.hpp"

#include <zookeeper/zookeeper.h>

#include "connection_zk.hpp"

namespace zk
{

connection::~connection() noexcept
{ }

std::shared_ptr<connection> connection::create(string_view conn_string)
{
    return std::make_shared<connection_zk>(conn_string);
}

}
