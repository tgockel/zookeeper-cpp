#include <zk/tests/test.hpp>
#include <zk/client.hpp>

#include <chrono>
#include <thread>

#include "package_registry_tests.hpp"
#include "server.hpp"
#include "server_tests.hpp"

namespace zk::server
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// server_fixture                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void server_fixture::SetUp()
{
    _server = server::create(test_package_registry::instance());
    _conn_string = "zk://127.0.0.1:2181";
}

void server_fixture::TearDown()
{
    _server->shutdown();
    _server.reset();
    _conn_string.clear();
}

const std::string& server_fixture::get_connection_string() const
{
    return _conn_string;
}

client server_fixture::get_connected_client() const
{
    return client(client::create(get_connection_string()).get());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unit Tests                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(server_tests, start_stop)
{
    auto svr = server::create(test_package_registry::instance());
    std::this_thread::sleep_for(std::chrono::seconds(5));
    svr->shutdown();
}

}
