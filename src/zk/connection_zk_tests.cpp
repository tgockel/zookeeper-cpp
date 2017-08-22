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

GTEST_TEST_F(connection_zk_tests, get_root)
{
    client c = get_connected_client();
    auto f = c.get("/");
    auto r = f.get();
    std::cout << r.second << std::endl;
    c.close();
}

GTEST_TEST_F(connection_zk_tests, create)
{
    client c = get_connected_client();
    const char local_buf[10] = { 0 };
    auto f_create = c.create("/test-node", buffer(local_buf, local_buf + sizeof local_buf));
    std::string name(f_create.get());
    CHECK_EQ("/test-node", name);
}

}
