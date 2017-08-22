#include <zk/server/server_tests.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "client.hpp"
#include "connection_zk.hpp"
#include "error.hpp"

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

static buffer buffer_from(string_view str)
{
    return buffer(str.data(), str.data() + str.size());
}

GTEST_TEST_F(connection_zk_tests, create_seq_and_set)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    std::string name(f_create.get());
    std::pair<buffer, stat> contents_orig(c.get(name).get());
    auto expected_version = contents_orig.second.data_version;
    ++expected_version;

    c.set(name, buffer_from("WORLD")).get();
    std::pair<buffer, stat> contents(c.get(name).get());
    CHECK_EQ(contents.second.data_version, expected_version);
    CHECK_TRUE(contents.first == buffer_from("WORLD"));
}

GTEST_TEST_F(connection_zk_tests, create_seq_and_erase)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    std::string name(f_create.get());
    std::pair<buffer, stat> contents_orig(c.get(name).get());
    c.erase(name, contents_orig.second.data_version).get();
    CHECK_THROWS(no_node)
    {
        c.get(name).get();
    };
}

GTEST_TEST_F(connection_zk_tests, create_seq_and_get_children)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    std::string name(f_create.get());
    std::pair<buffer, stat> contents_orig(c.get(name).get());

    c.create(name + "/a", buffer()).get();
    c.create(name + "/b", buffer()).get();
    c.create(name + "/c", buffer()).get();

    std::pair<std::vector<std::string>, stat> children(c.get_children(name).get());
    CHECK_EQ(contents_orig.second.data_version,  children.second.data_version);
    CHECK_LT(contents_orig.second.child_version, children.second.child_version);
    std::vector<std::string> expected_children = { "a", "b", "c" };
    std::sort(children.first.begin(), children.first.end());
    CHECK_TRUE(expected_children == children.first);
}

}
