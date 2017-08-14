#include <zk/tests/test.hpp>

#include <chrono>
#include <thread>

#include "package_registry_tests.hpp"
#include "server.hpp"

namespace zk::server
{

GTEST_TEST(server_tests, start_stop)
{
    auto svr = server::create(test_package_registry::instance());
    std::this_thread::sleep_for(std::chrono::seconds(5));
    svr->shutdown();
}

}
