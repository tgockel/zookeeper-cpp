#include <zk/tests/test.hpp>
#include <zk/client.hpp>

#include <cerrno>
#include <chrono>
#include <iostream>
#include <system_error>
#include <thread>

#include <ftw.h>
#include <unistd.h>

#include "classpath.hpp"
#include "configuration.hpp"
#include "package_registry.hpp"
#include "package_registry_tests.hpp"
#include "server.hpp"
#include "server_tests.hpp"

namespace zk::server
{

void delete_directory(std::string path)
{
    auto unlink_cb = [] (ptr<const char> fpath, ptr<const struct ::stat>, int, ptr<struct FTW>) -> int
                     {
                         return std::remove(fpath);
                     };

    if (nftw(path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS))
    {
        if (errno == ENOENT)
            return;
        else
            throw std::system_error(errno, std::system_category());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// server_fixture                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void server_fixture::SetUp()
{
    delete_directory("zk-data");
    _server = std::make_shared<server>(test_package_registry::instance().find_newest_classpath().value(),
                                       configuration::make_minimal("zk-data")
                                      );
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

void server_fixture::stop_server(bool wait_for_stop)
{
    _server->shutdown(wait_for_stop);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// single_server_fixture                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::shared_ptr<server> single_server_server;
static std::string             single_server_conn_string;

void single_server_fixture::SetUpTestCase()
{
    delete_directory("zk-data");
    single_server_server = std::make_shared<server>(test_package_registry::instance().find_newest_classpath().value(),
                                                    configuration::make_minimal("zk-data")
                                                   );
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
    server svr(test_package_registry::instance().find_newest_classpath().value(),
               configuration::make_minimal("zk-data")
              );
    std::this_thread::sleep_for(std::chrono::seconds(1));
    svr.shutdown();
}

GTEST_TEST(server_tests, shutdown_and_wait)
{
    server svr(test_package_registry::instance().find_newest_classpath().value(),
               configuration::make_minimal("zk-data")
              );
    svr.shutdown(true);
}

}
