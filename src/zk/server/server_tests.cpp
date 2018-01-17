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
    return client(client::connect(get_connection_string()).get());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// single_server_fixture                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::shared_ptr<server> single_server_server;
static std::string             single_server_conn_string;

void single_server_fixture::SetUpTestCase()
{
    single_server_server = server::create(test_package_registry::instance());
    single_server_conn_string = "zk://127.0.0.1:2181";
}

void single_server_fixture::TearDownTestCase()
{
    single_server_server->shutdown();
    single_server_server.reset();
    single_server_conn_string.clear();
}

const std::string& single_server_fixture::get_connection_string()
{
    return single_server_conn_string;
}

client single_server_fixture::get_connected_client()
{
    return client(client::connect(get_connection_string()).get());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unit Tests                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(server_tests, start_stop)
{
    auto svr = server::create(test_package_registry::instance());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    svr->shutdown();
}

GTEST_TEST(server_tests, shutdown_and_wait)
{
    auto svr = server::create(test_package_registry::instance());
    svr->shutdown(true);
}

}
