#include <zk/tests/test.hpp>

#include "connection.hpp"

namespace zk
{

// This test is mostly to check that we still use the defaults.
GTEST_TEST(connection_params_tests, defaults)
{
    const auto res = connection_params::parse("zk://localhost/");
    CHECK_EQ("zk", res.connection_schema());
    CHECK_EQ(1U, res.hosts().size());
    CHECK_EQ("localhost", res.hosts()[0]);
    CHECK_EQ("/", res.chroot());
    CHECK_TRUE(res.randomize_hosts());
    CHECK_FALSE(res.read_only());
    CHECK_TRUE(connection_params::default_timeout == res.timeout());

    connection_params manual;
    manual.hosts() = { "localhost" };
    CHECK_EQ(manual, res);
}

GTEST_TEST(connection_params_tests, multi_hosts)
{
    const auto res = connection_params::parse("zk://server-a,server-b,server-c/");
    connection_params manual;
    manual.hosts() = { "server-a", "server-b", "server-c" };
    CHECK_EQ(manual, res);
}

GTEST_TEST(connection_params_tests, chroot)
{
    const auto res = connection_params::parse("zk://localhost/some/sub/path");
    connection_params manual;
    manual.hosts()  = { "localhost" };
    manual.chroot() = "/some/sub/path";
    CHECK_EQ(manual, res);
}

GTEST_TEST(connection_params_tests, randomize_hosts)
{
    const auto res = connection_params::parse("zk://localhost/?randomize_hosts=false");
    connection_params manual;
    manual.hosts()           = { "localhost" };
    manual.randomize_hosts() = false;
    CHECK_EQ(manual, res);
}

GTEST_TEST(connection_params_tests, read_only)
{
    const auto res = connection_params::parse("zk://localhost/?read_only=true");
    connection_params manual;
    manual.hosts()     = { "localhost" };
    manual.read_only() = true;
    CHECK_EQ(manual, res);
}

GTEST_TEST(connection_params_tests, randomize_and_read_only)
{
    const auto res = connection_params::parse("zk://localhost/?randomize_hosts=false&read_only=1");
    connection_params manual;
    manual.hosts()           = { "localhost" };
    manual.randomize_hosts() = false;
    manual.read_only()       = true;
    CHECK_EQ(manual, res);
}

GTEST_TEST(connection_params_tests, timeout)
{
    const auto res = connection_params::parse("zk://localhost/?timeout=0.5");
    connection_params manual;
    manual.hosts()   = { "localhost" };
    manual.timeout() = std::chrono::milliseconds(500);
    CHECK_EQ(manual, res);
}

}
