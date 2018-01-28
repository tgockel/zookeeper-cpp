#include <zk/client.hpp>
#include <zk/tests/test.hpp>

#include <chrono>
#include <thread>

#include "configuration.hpp"
#include "package_registry.hpp"
#include "package_registry_tests.hpp"
#include "server_group.hpp"

namespace zk::server
{

void delete_directory(std::string path);

GTEST_TEST(server_group_tests, ensemble)
{
    delete_directory("ensemble");
    auto group = server_group::make_ensemble(5U, configuration::make_minimal("ensemble"));
    group.start_all_servers(test_package_registry::instance().find_newest_classpath().value());

    // connect and get data from the ensemble
    auto c = client::connect(group.get_connection_string()).get();
    CHECK_TRUE(c.exists("/").get());
}

}
