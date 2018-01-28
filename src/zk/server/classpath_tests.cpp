#include <zk/tests/test.hpp>

#include <iostream>
#include <stdexcept>

#include "classpath.hpp"

namespace zk::server
{

GTEST_TEST(classpath_tests, system_default)
{
    try
    {
        auto cp = classpath::system_default();
        std::cout << "Found system-default ZooKeeper: " << cp.command_line() << std::endl;
    }
    catch (const std::runtime_error& ex)
    {
        std::cout << "Could not find system-default ZooKeeper: " << ex.what() << std::endl;
    }
}

}
