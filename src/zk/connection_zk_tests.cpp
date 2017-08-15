#include <zk/server/server_tests.hpp>

#include "client.hpp"
#include "connection_zk.hpp"

namespace zk
{

class connection_zk_tests :
        public server::server_fixture
{ };

GTEST_TEST_F(connection_zk_tests, wat)
{
    client c(get_connection_string());
    c.close();
}

}
