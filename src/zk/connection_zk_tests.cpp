#include <zk/server/server_tests.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "client.hpp"
#include "connection_zk.hpp"

namespace zk
{

class connection_zk_tests :
        public server::server_fixture
{ };

GTEST_TEST_F(connection_zk_tests, wat)
{
    client c = get_connected_client();
    auto f = c.get("/");
    auto r = f.get();
    std::cout << r.second << std::endl;
    c.close();
}

}
